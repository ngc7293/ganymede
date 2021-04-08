#include <memory>
#include <filesystem>
#include <sstream>
#include <fstream>

#include <grpcpp/grpcpp.h>

#include "common/auth/jwt.hh"
#include "common/log.hh"
#include "common/status.hh"

#include "measurements.pb.h"
#include "measurements.grpc.pb.h"
#include "measurements.service_config.pb.h"

#include "influxdb.hh"

namespace ganymede::services::measurements {

class MeasurementsServiceImpl final : public MeasurementsService::Service {
public:
    MeasurementsServiceImpl() = delete;

    MeasurementsServiceImpl(influx::InfluxDB&& influx)
        : m_influx(std::move(influx))
    {
        m_influx.SetBatchSize(1000);
    }

    grpc::Status PushMeasurements(grpc::ServerContext* context, const PushMeasurementsRequest* request, Empty* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        for (const Measurement& measurement : request->measurements()) {
            const std::string& source = measurement.source_uid();
            const auto timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(measurement.timestamp())));

            if (measurement.has_atmosphere()) {
                const AtmosphericMeasurements& atmo = measurement.atmosphere();
                std::vector<influx::value_pair> data;

                if (atmo.temperature() != 0) {
                    data.emplace_back("temperature", atmo.temperature());
                }

                if (atmo.humidity() != 0) {
                    data.emplace_back("humidity", atmo.humidity());
                }

                if (not data.empty()) {
                    m_influx.Write({{"atmo", {{"domain", domain}, {"source", source}}, std::vector<influx::value_pair>(data), timestamp}});
                }
            }

            if (measurement.has_solution()) {
                const SolutionMeasurements& solution = measurement.solution();
                std::vector<influx::value_pair> data;

                if (solution.temperature() != 0) {
                    data.emplace_back("temperature", solution.temperature());
                }

                if (solution.flow() != 0) {
                    data.emplace_back("flow", solution.flow());
                }

                if (solution.ph() != 0) {
                    data.emplace_back("ph", solution.ph());
                }

                if (solution.ec() != 0) {
                    data.emplace_back("ec", solution.ec());
                }

                if (not data.empty()) {
                    m_influx.Write({{"solution", {{"domain", domain}, {"source", source}}, std::vector<influx::value_pair>(data), timestamp}});
                }
            }
        }

        m_influx.Flush();
        return grpc::Status::OK;
    }

    grpc::Status GetMeasurements(grpc::ServerContext* context, const GetMeasurementsRequest* request, GetMeasurementsResponse* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        std::ostringstream os;
        os << R"(from(bucket: "measurements"))" << std::endl;
        os << R"(  |> range(start: -4h))" << std::endl;
        os << R"(  |> filter(fn: (r) => r["_measurement"] == "atmo" or r["_measurement"] == "solution"))" << std::endl;
        os << R"(  |> filter(fn: (r) => r["_field"] == "ec" or r["_field"] == "flow" or r["_field"] == "humidity" or r["_field"] == "ph" or r["_field"] == "temperature"))" << std::endl;
        os << R"(  |> filter(fn: (r) => r["domain"] == ")" << domain << "\")" << std::endl;
        os << R"(  |> filter(fn: (r) => r["source"] == ")" << request->source_uid() << "\")" << std::endl;
        os << R"(  |> aggregateWindow(every: 5s, fn: mean, createEmpty: false))" << std::endl;
        os << R"(  |> yield(name: "mean"))" << std::endl;
        std::vector<influx::Point> points = m_influx.Query(os.str());

        std::map<std::chrono::time_point<std::chrono::high_resolution_clock>, Measurement> measurements;
        common::log::info({{"message", "parsing points"}, {"size", points.size()}});
        for (const auto& point : points) {
            Measurement& m = measurements[point.time()];

            m.set_source_uid(request->source_uid());
            m.set_timestamp(std::chrono::duration_cast<std::chrono::seconds>(point.time().time_since_epoch()).count());

            for (const auto& value : point.values()) {
                if (value.first == "temperature") {
                    if (point.series() == "solution") {
                        m.mutable_solution()->set_temperature(std::get<double>(value.second));
                    } else if (point.series() == "atmo") {
                        m.mutable_atmosphere()->set_temperature(std::get<double>(value.second));
                    }
                } else if (value.first == "humidity") {
                    m.mutable_atmosphere()->set_humidity(std::get<double>(value.second));
                } else if (value.first == "ph") {
                    m.mutable_solution()->set_ph(std::get<double>(value.second));
                } else if (value.first == "ec") {
                    m.mutable_solution()->set_ec(std::get<double>(value.second));
                }
            }
        }

        common::log::info({{"message", "building measurements response"}, {"size", measurements.size()}});
        for (auto it = measurements.begin(); it != measurements.end(); it++) {
            if (not it->second.has_atmosphere() and not it->second.has_solution()) {
                continue;
            }
            response->mutable_measurements()->Add(Measurement(it->second));
        }

        common::log::info({{"message", "returning measurements"}, {"size", response->measurements().size()}});
        return grpc::Status::OK;
    }

private:
    influx::InfluxDB m_influx;
};


}

std::string readFile(const std::filesystem::path& path)
{
    std::ostringstream content("");
    std::ifstream ifs(path);

    if (ifs) {
        content << ifs.rdbuf();
        ifs.close();
    }

    return content.str();
}


int main(int argc, const char* argv[])
{
    ganymede::services::measurements::MeasurementsServiceConfig config;
    google::protobuf::util::JsonStringToMessage(readFile(argv[1]), &config);

    ganymede::services::measurements::MeasurementsServiceImpl service(
        ganymede::influx::InfluxDB(config.influxdb().host(), config.influxdb().organization(), config.influxdb().bucket(), config.influxdb().token())
    );

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    ganymede::common::log::info({{"message", "listening on 0.0.0.0:3000"}});

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();

    return 0;
}