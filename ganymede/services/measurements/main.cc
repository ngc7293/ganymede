#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include <grpcpp/grpcpp.h>

#include <influx/influx.hh>

#include "common/auth/jwt.hh"
#include "common/log.hh"
#include "common/status.hh"

#include "measurements.pb.h"
#include "measurements.grpc.pb.h"
#include "measurements.service_config.pb.h"

namespace ganymede::services::measurements {

class MeasurementsServiceImpl final : public MeasurementsService::Service {
public:
    MeasurementsServiceImpl(influx::Influx&& influx)
        : influx_(influx)
    {
    }

    grpc::Status PushMeasurements(grpc::ServerContext* context, const PushMeasurementsRequest* request, Empty* response) override {
        std::string domain;
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        influx::Bucket bucket;
        try {
            bucket = influx_["measurements-" + domain];
        } catch (const influx::InfluxRemoteError& err) {
            common::log::error({{"message", err.what()}, {"code", err.statusCode()}});
        } catch (const std::exception& err) {
            common::log::error({{"message", err.what()}});
            return grpc::Status(grpc::StatusCode::INTERNAL, "");
        }

        if (!bucket) {
            return common::status::DATABASE_ERROR;
        }

        for (const Measurement& measurement: request->measurements()) {
            const std::string& source = measurement.source_uid();
            const influx::Timestamp timestamp = influx::Timestamp(std::chrono::seconds(measurement.timestamp()));

            if (measurement.has_atmosphere()) {
                const AtmosphericMeasurements& atmo = measurement.atmosphere();
                influx::Measurement record("atmo", timestamp);
                record << influx::Tag("source", source);

                if (atmo.temperature() != 0) {
                    record << influx::Field("temperature", atmo.temperature());
                }

                if (atmo.humidity() != 0) {
                    record << influx::Field("humidity", atmo.humidity());
                }

                if (record.fields().size()) {
                    bucket << record;
                }
            }

            if (measurement.has_solution()) {
                const SolutionMeasurements& solution = measurement.solution();
                influx::Measurement record("solution", timestamp);
                record << influx::Tag("source", source);

                if (solution.temperature() != 0) {
                    record << influx::Field("temperature", solution.temperature());
                }

                if (solution.flow() != 0) {
                    record << influx::Field("flow", solution.flow());
                }

                if (solution.ph() != 0) {
                    record << influx::Field("ph", solution.ph());
                }

                if (solution.ec() != 0) {
                    record << influx::Field("ec", solution.ec());
                }

                if (record.fields().size()) {
                    bucket << record;
                }
            }
        }

        try {
            bucket.Flush();
        } catch (const std::exception& err) {
            common::log::error({{"message", err.what()}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

    grpc::Status GetMeasurements(grpc::ServerContext* context, const GetMeasurementsRequest* request, GetMeasurementsResponse* response) override {
        std::string domain;
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }


        // TODO: This is not great, maybe use {fmt}
        std::ostringstream os;
        os << "from(bucket: \"measurements-" << domain << "\")" << std::endl;
        os << R"(  |> range(start: -4h))" << std::endl;
        os << R"(  |> filter(fn: (r) => r["_measurement"] == "atmo" or r["_measurement"] == "solution"))" << std::endl;
        os << R"(  |> filter(fn: (r) => r["_field"] == "ec" or r["_field"] == "flow" or r["_field"] == "humidity" or r["_field"] == "ph" or r["_field"] == "temperature"))" << std::endl;
        os << R"(  |> filter(fn: (r) => r["source"] == ")" << request->source_uid() << "\")" << std::endl;
        os << R"(  |> aggregateWindow(every: 5s, fn: mean, createEmpty: false))" << std::endl;
        os << R"(  |> yield(name: "mean"))" << std::endl;

        try {
            std::unordered_map<std::uint64_t, Measurement*> measurements;
            auto result = influx_.Query(os.str());
            for (const auto& table: result) { for (const auto& record: table) {
                std::uint64_t ts = std::chrono::duration_cast<std::chrono::seconds>(record.time.time_since_epoch()).count();
                Measurement* measurement = nullptr;

                if (measurements.contains(ts)) {
                    measurement = measurements[ts];
                } else {
                    measurement = response->add_measurements();
                    measurement->set_source_uid(request->source_uid());
                    measurement->set_timestamp(ts);
                    measurements[ts] = measurement;
                }

                if (record.measurement == "atmo") {
                    if (record.field == "temperature") {
                        measurement->mutable_atmosphere()->set_temperature(std::get<double>(record.value));
                    } else if (record.field == "humidity") {
                        measurement->mutable_atmosphere()->set_humidity(std::get<double>(record.value));
                    }
                } else if (record.measurement == "solution") {
                    if (record.field == "flow") {
                        measurement->mutable_solution()->set_flow(std::get<double>(record.value));
                    } else if (record.field == "temperature") {
                        measurement->mutable_solution()->set_temperature(std::get<double>(record.value));
                    } else if (record.field == "ph") {
                        measurement->mutable_solution()->set_ph(std::get<double>(record.value));
                    } else if (record.field == "ec") {
                        measurement->mutable_solution()->set_ec(std::get<double>(record.value));
                    }
                }
            }}
        } catch (const std::exception& err) {
            common::log::error({{"message", err.what()}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

private:
    influx::Influx influx_;
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
    if (argc <= 1) {
        ganymede::common::log::error({{"message", "Missing configuration file argument"}});
        return -1;
    }

    ganymede::services::measurements::MeasurementsServiceConfig config;
    google::protobuf::util::JsonStringToMessage(readFile(argv[1]), &config);

    influx::Influx influx(config.influxdb().host(), config.influxdb().organization(), config.influxdb().token());
    ganymede::services::measurements::MeasurementsServiceImpl service(std::move(influx));

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    ganymede::common::log::info({{"message", "listening on 0.0.0.0:3000"}});

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();

    return 0;
}