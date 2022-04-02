#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <google/protobuf/util/json_util.h>

#include <grpcpp/grpcpp.h>

#include <mongoc/mongoc.h>
#include <mongocxx/instance.hpp>

#include <ganymede/log/log.hh>
#include <ganymede/mongo/protobuf_collection.hh>

#include "device_service_impl.hh"

std::string makeMongoURI(const ganymede::services::device::DeviceServiceConfig::MongoDBConfig& config)
{
    std::stringstream ss;
    ss << "mongodb+srv://"
       << config.user() << ":" << config.password()
       << "@" << config.host()
       << "/" << config.database()
       << "?retryWrites=true&w=majority";

    return ss.str();
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
    mongoc_init();

    if (argc < 2) {
        return -1;
    }

    ganymede::services::device::DeviceServiceConfig service_config;
    if (not google::protobuf::util::JsonStringToMessage(readFile(argv[1]), &service_config).ok()) {
        return -1;
    }

    std::string mongo_uri = makeMongoURI(service_config.mongodb());
    ganymede::services::device::DeviceServiceImpl service(mongo_uri);
    mongocxx::instance instance{};

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    ganymede::log::info({ { "message", "listening on 0.0.0.0:3000" } });

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();

    mongoc_cleanup();
    return 0;
}