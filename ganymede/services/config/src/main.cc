#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <filesystem>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

#include <mongoc/mongoc.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "common/status.hh"
#include "common/auth/jwt.hh"

#include "config.pb.h"
#include "config.grpc.pb.h"
#include "config.service_config.pb.h"

#include "bson_serde.hh"

namespace {

bsoncxx::document::value BuildIDFilter(const std::string& id, const std::string& domain)
{
    bsoncxx::builder::basic::document builder;
    builder.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(std::string_view(id))));
    builder.append(bsoncxx::builder::basic::kvp("domain", domain));
    return builder.extract();
}

}

namespace ganymede::services::config {

class ConfigServiceImpl final : public ConfigService::Service {
public:
    ConfigServiceImpl() = delete;
    ConfigServiceImpl(std::string mongo_uri)
        : m_mongoclient(mongocxx::uri{mongo_uri})
        , m_mongodb(m_mongoclient["configurations"])
        , m_collection(m_mongodb["configurations"])
    {
    }

    grpc::Status CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, CreateConfigResponse* response) {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (not request->has_config()){
            return common::status::BAD_PAYLOAD;
        }

        auto builder = bsoncxx::builder::basic::document{};
        builder.append(bsoncxx::builder::basic::kvp("domain", domain));

        if (not MessageToBson(request->config(), builder)) {
            return common::status::UNIMPLEMENTED;
        }

        bsoncxx::stdx::optional<mongocxx::result::insert_one> result;
        try {
             result = m_collection.insert_one(builder.view());
        } catch (const mongocxx::exception& e) {
            std::cerr << "[error]" << e.what() << std::endl;
            return common::status::DATABASE_ERROR;
        }

        if (not result or result.value().result().inserted_count() != 1) {
            return common::status::DATABASE_ERROR;
        }
        response->set_uid(result.value().inserted_id().get_oid().value.to_string());
        return grpc::Status::OK;
    }

    grpc::Status GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, GetConfigResponse* response) {
        std::string domain;
        
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        bsoncxx::stdx::optional<bsoncxx::document::value> result;
        try {
            result = m_collection.find_one(BuildIDFilter(request->uid(), domain));
        } catch (const mongocxx::exception& e) {
            std::cerr << R"({"severity":"ERROR", "message":")" << e.what() << "\"}" << std::endl;
            return common::status::DATABASE_ERROR;
        }

        if (not result) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
        }

        BsonToMessage(result.value(), *(response->mutable_config()));
        return grpc::Status::OK;
    }

    grpc::Status UpdateConfig(grpc::ServerContext* context, const UpdateConfigRequest* request, UpdateConfigResponse* response) {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (not request->has_config()){
            return common::status::BAD_PAYLOAD;
        }

        auto builder = bsoncxx::builder::basic::document{};
        if (not MessageToBson(request->config(), builder)) {
            return common::status::UNIMPLEMENTED;
        }

        bsoncxx::stdx::optional<mongocxx::result::update> result;
        try {
            result = m_collection.update_one(BuildIDFilter(request->uid(), domain), builder.view());
        } catch (const mongocxx::exception& e) {
            std::cerr << "[error]" << e.what() << std::endl;
            return common::status::DATABASE_ERROR;
        }

        if (not result or result.value().modified_count() == 0) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
        }

        return grpc::Status::OK;
    }

private:
     mongocxx::client m_mongoclient;
     mongocxx::database m_mongodb;
     mongocxx::collection m_collection;
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