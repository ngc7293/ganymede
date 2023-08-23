#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <optional>

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
    builder.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(std::string(id))));
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
        if (!valid) {
            return false;
        }
    }

    return true;
}

bool ValidateIsOid(const std::string& id)
{
    try {
        bsoncxx::oid(std::string(id));
        return true;
    } catch (...) {
        return false;
    }
}

}

namespace ganymede::services::device_config {

class DeviceConfigServiceImpl final : public DeviceConfigService::Service {
public:
    DeviceConfigServiceImpl() = delete;
    DeviceConfigServiceImpl(std::string mongo_uri)
        : m_mongoclient(mongocxx::uri{mongo_uri})
        , m_mongodb(m_mongoclient["configurations"])
        , m_device_collection(m_mongodb["devices"])
        , m_config_collection(m_mongodb["configurations"])
    {
        CreateUniqueIndex(m_device_collection, std::to_string(Device::kMacFieldNumber));
    }

    grpc::Status AddDevice(grpc::ServerContext* context, const AddDeviceRequest* request, Device* response) override {
        std::string domain;
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (!request->has_device()) {
            return common::status::BAD_PAYLOAD;
        }

        Device device = RemoveUIDFromMessage(request->device());

        if (device.mac().empty() || (!ValidateIsMacAddress(device.mac()))) {
            return common::status::BAD_PAYLOAD;
        }

        if (!device.config_uid().empty())
            if (!(ValidateIsOid(device.config_uid()) && CheckIfExistInMongo(m_config_collection, BuildIDAndDomainFilter(device.config_uid(), domain)))) {
            return common::status::BAD_PAYLOAD;
        }

        std::string id;
        auto builder = DocumentWithDomain(domain);

        if (!MessageToBson(device, builder)) {
            return common::status::UNIMPLEMENTED;
        }

        int ec;
        if (auto status = TryInsertDocumentIntoMongo(m_device_collection, builder, id, ec)) {
            if (ec == MONGO_UNIQUE_KEY_VIOLATION) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "there is already a Device with this MAC");
            }
            return status.value();
        }

        if (!FetchFromMongo<Device>(m_device_collection, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "inserted device but could !fetch it back"}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

    grpc::Status GetDevice(grpc::ServerContext* context, const GetDeviceRequest* request, Device* response) override {
        std::string domain;

        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        try {
            auto filter = DocumentWithDomain(domain);
            switch(request->filter_case()) {
            case GetDeviceRequest::kDeviceUid:
                if (ValidateIsOid(request->device_uid())) {
                    filter.append(bsoncxx::builder::basic::kvp("_id", request->device_uid()));
                } else {
                    return common::status::BAD_PAYLOAD;
                }
                break;
            case GetDeviceRequest::kDeviceMac:
                if (ValidateIsMacAddress(request->device_mac())) {
                    filter.append(bsoncxx::builder::basic::kvp(std::to_string(GetDeviceRequest::kDeviceMacFieldNumber), request->device_mac()));
                } else {
                    return common::status::BAD_PAYLOAD;
                }
                break;
            case GetDeviceRequest::FILTER_NOT_SET:
                return common::status::BAD_PAYLOAD;
            }

            if (!FetchFromMongo<Device>(m_device_collection, filter.view(), response)) {
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
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (!request->has_device()) {
            return common::status::BAD_PAYLOAD;
        }

        std::string id = request->device().uid();
        const Device& device = RemoveUIDFromMessage(request->device());

        if (!ValidateIsOid(id)) {
            return common::status::BAD_PAYLOAD;
        }

        if (!device.config_uid().empty())
            if (!(ValidateIsOid(device.config_uid()) && CheckIfExistInMongo(m_config_collection, BuildIDAndDomainFilter(device.config_uid(), domain)))) {
            return common::status::BAD_PAYLOAD;
        }

        auto builder = bsoncxx::builder::basic::document{};
        auto nested = bsoncxx::builder::basic::document{};
        if (!MessageToBson(device, nested)) {
            return common::status::UNIMPLEMENTED;
        }
        builder.append(bsoncxx::builder::basic::kvp("$set", nested));

        bsoncxx::stdx::optional<mongocxx::result::update> result;
        try {
            result = m_device_collection.update_one(BuildIDAndDomainFilter(id, domain), builder.view());
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (!result || result.value().modified_count() == 0) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
        }

        if (!FetchFromMongo<Device>(m_device_collection, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "updated config but could !fetch it back"}});
            return common::status::DATABASE_ERROR;
        }
        return grpc::Status::OK;
    }

    grpc::Status RemoveDevice(grpc::ServerContext* context, const RemoveDeviceRequest* request, Empty* response) override {
        std::string domain;
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (!ValidateIsOid(request->device_uid())) {
            return common::status::BAD_PAYLOAD;
        }

        bsoncxx::stdx::optional<mongocxx::result::delete_result> result;

        try {
            result = m_device_collection.delete_one(BuildIDAndDomainFilter(request->device_uid(), domain));
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (!result.has_value() || result.value().deleted_count() != 1) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
        }

        return grpc::Status::OK;
    }

    grpc::Status CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, Config* response) override {
        std::string domain;
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (!request->has_config()){
            return common::status::BAD_PAYLOAD;
        }

        std::string id;
        auto builder = DocumentWithDomain(domain);
        Config config = RemoveUIDFromMessage(request->config());

        if (!MessageToBson(config, builder)) {
            return common::status::UNIMPLEMENTED;
        }

        if (auto status = TryInsertDocumentIntoMongo(m_config_collection, builder, id)) {
            return status.value();
        }

        if (!FetchFromMongo<Config>(m_config_collection, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "inserted configuration but could !fetch it back"}});
            return common::status::DATABASE_ERROR;
        }

        return grpc::Status::OK;
    }

    grpc::Status GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, Config* response) override {
        std::string domain;
        
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (!ValidateIsOid(request->config_uid())) {
            return common::status::BAD_PAYLOAD;
        }

        try {
            if (!FetchFromMongo<Config>(m_config_collection, BuildIDAndDomainFilter(request->config_uid(), domain), response)) {
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
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (!request->has_config()) {
            return common::status::BAD_PAYLOAD;
        }

        std::string id = request->config().uid();
        const Config& config = RemoveUIDFromMessage(request->config());

        if (!ValidateIsOid(id)) {
            return common::status::BAD_PAYLOAD;
        }


        auto builder = bsoncxx::builder::basic::document{};
        auto nested = bsoncxx::builder::basic::document{};
        if (!MessageToBson(config, nested)) {
            return common::status::UNIMPLEMENTED;
        }
        builder.append(bsoncxx::builder::basic::kvp("$set", nested));

        bsoncxx::stdx::optional<mongocxx::result::update> result;
        try {
            result = m_config_collection.update_one(BuildIDAndDomainFilter(id, domain), builder.view());
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (!result || result.value().modified_count() == 0) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
        }

        if (!FetchFromMongo<Config>(m_config_collection, BuildIDAndDomainFilter(id, domain), response)) {
            common::log::error({{"message", "updated config but could !fetch it back"}});
            return common::status::DATABASE_ERROR;
        }
        return grpc::Status::OK;
    }

    grpc::Status DeleteConfig(grpc::ServerContext* context, const DeleteConfigRequest* request, Empty* response) override {
        std::string domain;
        if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
            return common::status::UNAUTHENTICATED;
        }

        if (!ValidateIsOid(request->config_uid())) {
            return common::status::BAD_PAYLOAD;
        }

        bsoncxx::stdx::optional<mongocxx::result::delete_result> result;

        try {
            result = m_config_collection.delete_one(BuildIDAndDomainFilter(request->config_uid(), domain));
        } catch (const mongocxx::exception& e) {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (!result.has_value() || result.value().deleted_count() != 1) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such configuration");
        }

        return grpc::Status::OK;
    }

private:
    std::optional<grpc::Status> TryInsertDocumentIntoMongo(mongocxx::collection& collection, const bsoncxx::builder::basic::document& builder, std::string& id) {
        int ec;
        return TryInsertDocumentIntoMongo(collection, builder, id, ec);
    }

    std::optional<grpc::Status> TryInsertDocumentIntoMongo(mongocxx::collection& collection, const bsoncxx::builder::basic::document& builder, std::string& id, int& ec) {
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result;

        try {
             result = collection.insert_one(builder.view());
        } catch (const mongocxx::exception& e) {
            ec = e.code().value();
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }

        if (!result || result.value().result().inserted_count() != 1) {
            return common::status::DATABASE_ERROR;
        }

        ec = 0;
        id = OIDToString(result->inserted_id());
        return std::optional<grpc::Status>();
    }

    template<class T>
    bool FetchFromMongo(mongocxx::collection& collection, const bsoncxx::document::view_or_value& filter, T* dest) {
        bsoncxx::stdx::optional<bsoncxx::document::value> result;

        result = collection.find_one(filter.view());

        if (result) {
            BsonToMessage(result.value(), *dest);
            dest->set_uid(GetDocumentID(result.value().view()));
        }
        return (result.has_value());
    }

    bool CheckIfExistInMongo(mongocxx::collection& collection, const bsoncxx::document::view_or_value& filter) {
        auto result = collection.count_documents(filter);
        return result > 0;
    }

    bool CreateUniqueIndex(mongocxx::collection& collection, const std::string& key) {
        auto keys = bsoncxx::builder::basic::document{};
        keys.append(bsoncxx::builder::basic::kvp(key, 1));

        auto constraints = bsoncxx::builder::basic::document{};
        constraints.append(bsoncxx::builder::basic::kvp("unique", true));

        collection.create_index(keys.view(), constraints.view());
        return true;
    }

private:
    mongocxx::client m_mongoclient;
    mongocxx::database m_mongodb;
    mongocxx::collection m_device_collection;
    mongocxx::collection m_config_collection;
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
    if (!google::protobuf::util::JsonStringToMessage(readFile(argv[1]), &service_config).ok()) {
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