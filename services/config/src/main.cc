#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <filesystem>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "config.pb.h"
#include "config.grpc.pb.h"
#include "config.service_config.pb.h"

namespace ganymede::services::config {

class ConfigServiceImpl final : public ConfigService::Service {
public:
    ConfigServiceImpl() = delete;
    ConfigServiceImpl(std::string mongo_uri)
        : m_mongoclient(mongocxx::uri{mongo_uri})
        , m_mongodb(m_mongoclient["configurations"])
    {
    }

    grpc::Status CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, CreateConfigResponse* response) {
        auto collection = m_mongodb["configurations"];
        auto builder = bsoncxx::builder::stream::document{};

        bsoncxx::document::value doc_value = builder
        << "uid" << 1
        << bsoncxx::builder::stream::finalize;

        try {
            bsoncxx::stdx::optional<mongocxx::result::insert_one> result = collection.insert_one(doc_value.view());

            if (not (result and result.value().result().inserted_count() == 1)) {
                return grpc::Status(grpc::StatusCode::UNKNOWN, "database error");
            }
        } catch (const mongocxx::exception& e) {
            return grpc::Status(grpc::StatusCode::UNKNOWN, "database error");
        }

        return grpc::Status::OK;
    }

    grpc::Status GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, GetConfigResponse* response) {
        return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not yet implemented");
    }

    grpc::Status UpdateConfig(grpc::ServerContext* context, const UpdateConfigRequest* request, UpdateConfigResponse* response) {
        return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not yet implemented");
    }

private:
     mongocxx::client m_mongoclient;
     mongocxx::database m_mongodb;
};


std::string makeMongoURI(const ConfigServiceConfig::MongoDBConfig& config)
{
    std::stringstream ss;
    ss << "mongodb+srv://" 
       << config.user() << ":" << config.password() 
       << "@" << config.host() 
       << "/" << config.database()
       << "?retryWrites=true&w=majority";

    return ss.str();
}

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
    ganymede::services::config::ConfigServiceConfig service_config;

    if (not google::protobuf::util::JsonStringToMessage(readFile("config.service_config.json"), &service_config).ok()) {
        return -1;
    }

    std::string mongo_uri = makeMongoURI(service_config.mongodb());
    ganymede::services::config::ConfigServiceImpl service(mongo_uri);
    mongocxx::instance instance{};

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();
    return 0;
}