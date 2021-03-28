#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <filesystem>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

#include <mongoc/mongoc.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/exception/error_code.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "common/log.hh"
#include "common/status.hh"
#include "common/auth/jwt.hh"

#include "device_config.pb.h"
#include "device_config.grpc.pb.h"
#include "device_config.service_config.pb.h"

#include "bson_serde.hh"

namespace {

const int MONGO_UNIQUE_KEY_VIOLATION = 11000;

bsoncxx::builder::basic::document DocumentWithDomain(const std::string& domain)
{
    auto builder = bsoncxx::builder::basic::document{};
    builder.append(bsoncxx::builder::basic::kvp("domain", domain));
    return builder;
}

bsoncxx::document::value BuildIDAndDomainFilter(const std::string& id, const std::string& domain)
{
    auto builder = DocumentWithDomain(domain);
    builder.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(std::string_view(id))));
    return builder.extract();
}

std::string OIDToString(const bsoncxx::types::bson_value::view& val)
{
    return val.get_oid().value.to_string();
}

std::string GetDocumentID(const bsoncxx::document::view_or_value& view)
{
    return OIDToString(view.view()["_id"].get_value());
}

template<class T>
T RemoveUIDFromMessage(const T& message)
{
    T result(message);
    if (result.uid() != "") {
        result.set_uid("");
    }
    return result;
}

bool ValidateIsMacAddress(const std::string& mac)
{
    size_t count = 0;

    if (mac.length() != 17) {
        return false;
    }

    for (const char& c : mac) {
        bool valid = ++count % 3 ? std::isxdigit(c) : (c == ':');
        if (not valid) {
            return false;
        }
    }

    return true;
}

}

namespace ganymede::services::device_config {

class DeviceConfigServiceImpl final : public DeviceConfigService::Service {
public:
    DeviceConfigServiceImpl() = delete;
    DeviceConfigServiceImpl(std::string mongo_uri)
        : m_mongoclient(mongocxx::uri{mongo_uri})
        , m_mongodb(m_mongoclient["configurations"])
    {
        CreateUniqueIndex(DEVICE_COLLECTION, std::to_string(Device::kMacFieldNumber));
    }

    grpc::Status AddDevice(grpc::ServerContext* context, const AddDeviceRequest* request, Device* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (not request->has_device() or not ValidateIsMacAddress(request->device().mac())) {
            return common::status::BAD_PAYLOAD;
        }

        std::string id;
        auto builder = DocumentWithDomain(domain);
        Device config = RemoveUIDFromMessage(request->device());

        if (not MessageToBson(config, builder)) {
            return common::status::UNIMPLEMENTED;
        }

        int ec;
        if (auto status = TryInsertDocumentIntoMongo(DEVICE_COLLECTION, builder, id, ec)) {
            if (ec == MONGO_UNIQUE_KEY_VIOLATION) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "there is already a Device with this MAC");
            }
            return status.value();
        }

        if (not FetchFromMongo<Device>(DEVICE_COLLECTION, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "inserted device but could not fetch it back"}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

    grpc::Status GetDevice(grpc::ServerContext* context, const GetDeviceRequest* request, Device* response) override {
        std::string domain;

        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        try {
            auto filter = DocumentWithDomain(domain);
            switch(request->filter_case()) {
                case GetDeviceRequest::kDeviceUid:
                    filter.append(bsoncxx::builder::basic::kvp("_id", request->device_uid()));
                    break;
                case GetDeviceRequest::kDeviceMac:
                    filter.append(bsoncxx::builder::basic::kvp(std::to_string(GetDeviceRequest::kDeviceMacFieldNumber), request->device_mac()));
                    break;
                case GetDeviceRequest::FILTER_NOT_SET:
                    return common::status::BAD_PAYLOAD;
            }

            if (not FetchFromMongo<Device>(DEVICE_COLLECTION, filter.view(), response)) {
                return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
            }
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

    grpc::Status ListDevice(grpc::ServerContext* context, const ListDeviceRequest* request, ListDeviceResponse* response) override {
        return common::status::UNIMPLEMENTED;
    }

    grpc::Status UpdateDevice(grpc::ServerContext* context, const UpdateDeviceRequest* request, Device* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (not request->has_device()) {
            return common::status::BAD_PAYLOAD;
        }

        std::string id = request->device().uid();
        const Device& device = RemoveUIDFromMessage(request->device());

        mongocxx::collection collection = m_mongodb[DEVICE_COLLECTION];
        auto builder = bsoncxx::builder::basic::document{};
        auto nested = bsoncxx::builder::basic::document{};
        if (not MessageToBson(device, nested)) {
            return common::status::UNIMPLEMENTED;
        }
        builder.append(bsoncxx::builder::basic::kvp("$set", nested));

        bsoncxx::stdx::optional<mongocxx::result::update> result;
        try {
            result = collection.update_one(BuildIDAndDomainFilter(id, domain), builder.view());
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (not result or result.value().modified_count() == 0) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
        }

        if (not FetchFromMongo<Device>(DEVICE_COLLECTION, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "updated config but could not fetch it back"}});
            return common::status::DATABASE_ERROR;
        }
        return grpc::Status::OK;
    }

    grpc::Status RemoveDevice(grpc::ServerContext* context, const RemoveDeviceRequest* request, Empty* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        mongocxx::collection collection = m_mongodb[DEVICE_COLLECTION];
        bsoncxx::stdx::optional<mongocxx::result::delete_result> result;

        try {
            result = collection.delete_one(BuildIDAndDomainFilter(request->device_uid(), domain));
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (not result.has_value() or result.value().deleted_count() != 1) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
        }

        return grpc::Status::OK;
    }

    grpc::Status CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, Config* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (not request->has_config()){
            return common::status::BAD_PAYLOAD;
        }

        std::string id;
        auto builder = DocumentWithDomain(domain);
        Config config = RemoveUIDFromMessage(request->config());

        if (not MessageToBson(config, builder)) {
            return common::status::UNIMPLEMENTED;
        }

        if (auto status = TryInsertDocumentIntoMongo(CONFIG_COLLECTION, builder, id)) {
            return status.value();
        }

        if (not FetchFromMongo<Config>(CONFIG_COLLECTION, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "inserted configuration but could not fetch it back"}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

    grpc::Status GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, Config* response) override {
        std::string domain;
        
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        try {
            if (not FetchFromMongo<Config>(CONFIG_COLLECTION, BuildIDAndDomainFilter(request->config_uid(), domain), response)) {
                return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
            }
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

    grpc::Status ListConfig(grpc::ServerContext* context, const ListConfigRequest* request, ListConfigResponse* response) override {
        return common::status::UNIMPLEMENTED;
    }

    grpc::Status UpdateConfig(grpc::ServerContext* context, const UpdateConfigRequest* request, Config* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (not request->has_config()) {
            return common::status::BAD_PAYLOAD;
        }

        std::string id = request->config().uid();
        const Config& config = RemoveUIDFromMessage(request->config());

        mongocxx::collection collection = m_mongodb[CONFIG_COLLECTION];
        auto builder = bsoncxx::builder::basic::document{};
        auto nested = bsoncxx::builder::basic::document{};
        if (not MessageToBson(config, nested)) {
            return common::status::UNIMPLEMENTED;
        }
        builder.append(bsoncxx::builder::basic::kvp("$set", nested));

        bsoncxx::stdx::optional<mongocxx::result::update> result;
        try {
            result = collection.update_one(BuildIDAndDomainFilter(id, domain), builder.view());
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (not result or result.value().modified_count() == 0) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
        }

        if (not FetchFromMongo<Config>(CONFIG_COLLECTION, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "updated config but could not fetch it back"}});
            return common::status::DATABASE_ERROR;
        }
        return grpc::Status::OK;
    }

    grpc::Status DeleteConfig(grpc::ServerContext* context, const DeleteConfigRequest* request, Empty* response) override {
        std::string domain;
        if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        mongocxx::collection collection = m_mongodb[CONFIG_COLLECTION];
        bsoncxx::stdx::optional<mongocxx::result::delete_result> result;

        try {
            result = collection.delete_one(BuildIDAndDomainFilter(request->config_uid(), domain));
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (not result.has_value() or result.value().deleted_count() != 1) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
        }

        return grpc::Status::OK;
    }

private:
    std::optional<grpc::Status> TryInsertDocumentIntoMongo(const std::string& collection_name, const bsoncxx::builder::basic::document& builder, std::string& id) {
        int ec;
        return TryInsertDocumentIntoMongo(collection_name, builder, id, ec);
    }

    std::optional<grpc::Status> TryInsertDocumentIntoMongo(const std::string& collection_name, const bsoncxx::builder::basic::document& builder, std::string& id, int& ec) {
        mongocxx::collection collection = m_mongodb[collection_name];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result;

        try {
             result = collection.insert_one(builder.view());
        } catch (const mongocxx::exception& e) {
            ec = e.code().value();
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (not result or result.value().result().inserted_count() != 1) {
            return common::status::DATABASE_ERROR;
        }

        ec = 0;
        id = OIDToString(result->inserted_id());
        return std::optional<grpc::Status>();
    }

    template<class T>
    bool FetchFromMongo(const std::string& collection_name, const bsoncxx::document::view_or_value& filter, T* dest) {
        mongocxx::collection collection = m_mongodb[collection_name];
        bsoncxx::stdx::optional<bsoncxx::document::value> result;

        result = collection.find_one(filter.view());

        if (result) {
            BsonToMessage(result.value(), *dest);
            dest->set_uid(GetDocumentID(result.value().view()));
        }
        return (result.has_value());
    }

    bool CreateUniqueIndex(const std::string& collection_name, const std::string& key) {
        mongocxx::collection collection = m_mongodb[collection_name];

        auto keys = bsoncxx::builder::basic::document{};
        keys.append(bsoncxx::builder::basic::kvp(key, 1));

        auto constraints = bsoncxx::builder::basic::document{};
        constraints.append(bsoncxx::builder::basic::kvp("unique", true));

        collection.create_index(keys.view(), constraints.view());
        return true;
    }

public:
    inline static const std::string DEVICE_COLLECTION = std::string("devices");
    inline static const std::string CONFIG_COLLECTION = std::string("configurations");

private:
     mongocxx::client m_mongoclient;
     mongocxx::database m_mongodb;
};


std::string makeMongoURI(const DeviceConfigServiceConfig::MongoDBConfig& config)
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

    ganymede::services::device_config::DeviceConfigServiceConfig service_config;
    if (not google::protobuf::util::JsonStringToMessage(readFile(argv[1]), &service_config).ok()) {
        return -1;
    }

    std::string mongo_uri = ganymede::services::device_config::makeMongoURI(service_config.mongodb());
    ganymede::services::device_config::DeviceConfigServiceImpl service(mongo_uri);
    mongocxx::instance instance{};

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    ganymede::common::log::info({{"message", "listening on 0.0.0.0:3000"}});

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();

    mongoc_cleanup();
    return 0;
}