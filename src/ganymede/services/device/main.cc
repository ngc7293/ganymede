#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <google/protobuf/util/json_util.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <mongoc/mongoc.h>

#include <ganymede/api/result.hh>
#include <ganymede/log/log.hh>
#include <ganymede/mongo/protobuf_collection.hh>

#include "device_service_impl.hh"

namespace {

void InitGlobals()
{
    mongoc_init();
    ganymede::log::Log::get().AddSink(std::make_unique<ganymede::log::StdIoLogSink>());
}

std::string ReadFile(const std::filesystem::path& path)
{
    std::ostringstream content("");
    std::ifstream ifs(path);

    if (ifs) {
        content << ifs.rdbuf();
        ifs.close();
    }

    return content.str();
}

ganymede::api::Result<ganymede::services::device::DeviceServiceConfig> BuildConfig(const std::vector<std::string>& args)
{
    ganymede::services::device::DeviceServiceConfig config;

    if (args.size() == 2) {
        if (not std::filesystem::is_regular_file(args[1])) {
            return { ganymede::api::Status::INVALID_ARGUMENT, "No such file: '" + args[1] + "'" };
        } else if (not google::protobuf::util::JsonStringToMessage(ReadFile(args[1]), &config).ok()) {
            return { ganymede::api::Status::INVALID_ARGUMENT, "Failed to parse file: '" + args[1] + "'" };
        }
    }

    if (config.mongo_uri().empty()) {
        config.set_mongo_uri("mongodb://mongo:27017/");
    }

    return { std::move(config) };
}

nlohmann::json ConfigAsJson(const ganymede::services::device::DeviceServiceConfig& config)
{
    std::string serialized;
    google::protobuf::util::MessageToJsonString(config, &serialized);
    return nlohmann::json::parse(serialized);
}

} // anonymous namespace

int main(int argc, const char* argv[])
{
    InitGlobals();

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.emplace_back(argv[i]);
    }

    auto result = BuildConfig(args);

    if (not result) {
        ganymede::log::error("could not build config: " + result.error());
        return -1;
    }

    auto service_config = result.value();
    ganymede::log::info("effective config", { { "config", ConfigAsJson(service_config) } });

    std::shared_ptr<ganymede::auth::AuthValidator> authValidator(new ganymede::auth::JwtAuthValidator());
    ganymede::services::device::DeviceServiceImpl service(service_config.mongo_uri(), authValidator);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    if (std::unique_ptr<grpc::Server> server = builder.BuildAndStart()) {
        ganymede::log::info("listening on 0.0.0.0:3000");

        grpc::HealthCheckServiceInterface* service = server->GetHealthCheckService();
        service->SetServingStatus("ganymede.services.device.DeviceService", true);

        server->Wait();
    }

    mongoc_cleanup();
    return 0;
}