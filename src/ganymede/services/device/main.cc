#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <google/protobuf/util/json_util.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <mongoc/mongoc.h>

#include <ganymede/log/log.hh>
#include <ganymede/mongo/protobuf_collection.hh>

#include "device_service_impl.hh"

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

void init_globals()
{
    mongoc_init();
    ganymede::log::Log::get().AddSink(std::make_unique<ganymede::log::StdIoLogSink>());
}

int main(int argc, const char* argv[])
{
    init_globals();

    if (argc < 2) {
        return -1;
    }

    ganymede::services::device::DeviceServiceConfig service_config;
    if (not google::protobuf::util::JsonStringToMessage(readFile(argv[1]), &service_config).ok()) {
        return -1;
    }

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