#include <memory>

#include <grpcpp/grpcpp.h>

#include "config.pb.h"
#include "config.grpc.pb.h"

namespace ganymede::services::config {

class ConfigServiceImpl final : public ConfigService::Service {
    grpc::Status CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, CreateConfigResponse* response) {
        response->set_uid("1");
        return grpc::Status::OK;
    }

    grpc::Status GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, GetConfigResponse* response) {
        response->set_uid("1");
        return grpc::Status::OK;
    }

    grpc::Status UpdateConfig(grpc::ServerContext* context, const UpdateConfigRequest* request, UpdateConfigResponse* response) {
        response->set_uid("1");
        return grpc::Status::OK;
    }
};

}

int main(int argc, const char* argv[])
{
    ganymede::services::config::ConfigServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:443", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();
    return 0;
}