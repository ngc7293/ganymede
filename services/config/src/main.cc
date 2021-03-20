#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <filesystem>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

#include <mongoc/mongoc.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "config.pb.h"
#include "config.grpc.pb.h"
#include "config.service_config.pb.h"

#include "bson_serde.hh"

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
        auto builder = bsoncxx::builder::basic::document{};

        if (not request->has_config()){
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "bad payload");
        }

        if (not MessageToBson(request->config(), builder)) {
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not yet implemented");
        }

        try {
            bsoncxx::stdx::optional<mongocxx::result::insert_one> result = collection.insert_one(builder.extract().view());

            if (not (result and result.value().result().inserted_count() == 1)) {
                return grpc::Status(grpc::StatusCode::UNKNOWN, "database error");
            }

            response->set_uid(result.value().inserted_id().get_oid().value.to_string());
            return grpc::Status::OK;
        } catch (const mongocxx::exception& e) {
            std::cerr << "[error]" << e.what() << std::endl;
            return grpc::Status(grpc::StatusCode::UNKNOWN, "database error");
        }
    }

    grpc::Status GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, GetConfigResponse* response) {
        auto collection = m_mongodb["configurations"];

        try {
            bsoncxx::stdx::optional<bsoncxx::document::value> result = collection.find_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid(std::string_view(request->uid())) << bsoncxx::builder::stream::finalize);

            if (result) {
                BsonToMessage(result.value(), *(response->mutable_config()));
                return grpc::Status::OK;
            } else {
                return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
            }
        } catch (const mongocxx::exception& e) {
            std::cerr << "[error]" << e.what() << std::endl;
            return grpc::Status(grpc::StatusCode::UNKNOWN, "database error");
        }
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
    mongoc_init();

    if (argc < 2) {
        return -1;
    }

    ganymede::services::config::ConfigServiceConfig service_config;

    if (not google::protobuf::util::JsonStringToMessage(readFile(argv[1]), &service_config).ok()) {
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

    mongoc_cleanup();
    return 0;
}