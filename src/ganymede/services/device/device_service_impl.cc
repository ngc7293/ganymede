#include <memory>
#include <string>
#include <optional>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/exception/error_code.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>

#include <ganymede/common/auth/jwt.hh>
#include <ganymede/common/log.hh>
#include <ganymede/common/mongo/bson_serde.hh>
#include <ganymede/common/mongo/oid.hh>
#include <ganymede/common/status.hh>

#include "device_service_impl.hh"

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
    builder.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(id.c_str(), id.length())));
    return builder.extract();
}

template<class MessageType>
MessageType RemoveUIDFromMessage(const MessageType& message)
{
    MessageType result(message);
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

}

namespace ganymede::services::device {

DeviceServiceImpl::DeviceServiceImpl(std::string mongo_uri)
    : m_mongoclient(mongocxx::uri{mongo_uri})
    , m_mongodb(m_mongoclient["configurations"])
    , m_device_collection(m_mongodb["devices"])
    , m_config_collection(m_mongodb["configurations"])
{
    CreateUniqueIndex(m_device_collection, std::to_string(Device::kMacFieldNumber));
}

grpc::Status DeviceServiceImpl::AddDevice(grpc::ServerContext* context, const AddDeviceRequest* request, Device* response)
{
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
        if (!(common::mongo::ValidateIsOid(device.config_uid()) && CheckIfExistInMongo(m_config_collection, BuildIDAndDomainFilter(device.config_uid(), domain)))) {
        return common::status::BAD_PAYLOAD;
    }

    std::string id;
    auto builder = DocumentWithDomain(domain);

    if (!mongo::MessageToBson(device, builder)) {
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

grpc::Status DeviceServiceImpl::GetDevice(grpc::ServerContext* context, const GetDeviceRequest* request, Device* response)
{
    std::string domain;

    if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    try {
        auto filter = DocumentWithDomain(domain);
        switch(request->filter_case()) {
        case GetDeviceRequest::kDeviceUid:
            if (common::mongo::ValidateIsOid(request->device_uid())) {
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

grpc::Status DeviceServiceImpl::ListDevice(grpc::ServerContext* /* context */, const ListDeviceRequest* /* request */, ListDeviceResponse* /* response */)
{
    return common::status::UNIMPLEMENTED;
}

grpc::Status DeviceServiceImpl::UpdateDevice(grpc::ServerContext* context, const UpdateDeviceRequest* request, Device* response)
{
    std::string domain;
    if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (!request->has_device()) {
        return common::status::BAD_PAYLOAD;
    }

    std::string id = request->device().uid();
    const Device& device = RemoveUIDFromMessage(request->device());

    if (!common::mongo::ValidateIsOid(id)) {
        return common::status::BAD_PAYLOAD;
    }

    if (!device.config_uid().empty())
        if (!(common::mongo::ValidateIsOid(device.config_uid()) && CheckIfExistInMongo(m_config_collection, BuildIDAndDomainFilter(device.config_uid(), domain)))) {
        return common::status::BAD_PAYLOAD;
    }

    auto builder = bsoncxx::builder::basic::document{};
    auto nested = bsoncxx::builder::basic::document{};
    if (!mongo::MessageToBson(device, nested)) {
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

grpc::Status DeviceServiceImpl::RemoveDevice(grpc::ServerContext* context, const RemoveDeviceRequest* request, Empty* /* response */)
{
    std::string domain;
    if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (!common::mongo::ValidateIsOid(request->device_uid())) {
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

grpc::Status DeviceServiceImpl::CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, Config* response)
{
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

    if (!mongo::MessageToBson(config, builder)) {
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

grpc::Status DeviceServiceImpl::GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, Config* response)
{
    std::string domain;
    
    if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (!common::mongo::ValidateIsOid(request->config_uid())) {
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

grpc::Status DeviceServiceImpl::ListConfig(grpc::ServerContext* /* context */, const ListConfigRequest* /* request */, ListConfigResponse* /* response */)
{
    return common::status::UNIMPLEMENTED;
}

grpc::Status DeviceServiceImpl::UpdateConfig(grpc::ServerContext* context, const UpdateConfigRequest* request, Config* response)
{
    std::string domain;
    if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (!request->has_config()) {
        return common::status::BAD_PAYLOAD;
    }

    std::string id = request->config().uid();
    const Config& config = RemoveUIDFromMessage(request->config());

    if (!common::mongo::ValidateIsOid(id)) {
        return common::status::BAD_PAYLOAD;
    }


    auto builder = bsoncxx::builder::basic::document{};
    auto nested = bsoncxx::builder::basic::document{};
    if (!mongo::MessageToBson(config, nested)) {
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

grpc::Status DeviceServiceImpl::DeleteConfig(grpc::ServerContext* context, const DeleteConfigRequest* request, Empty* /* response */)
{
    std::string domain;
    if (!common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (!common::mongo::ValidateIsOid(request->config_uid())) {
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

std::optional<grpc::Status> DeviceServiceImpl::TryInsertDocumentIntoMongo(mongocxx::collection& collection, const bsoncxx::builder::basic::document& builder, std::string& id)
{
    int ec;
    return TryInsertDocumentIntoMongo(collection, builder, id, ec);
}

std::optional<grpc::Status> DeviceServiceImpl::TryInsertDocumentIntoMongo(mongocxx::collection& collection, const bsoncxx::builder::basic::document& builder, std::string& id, int& ec)
{
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
    id = common::mongo::OIDToString(result->inserted_id());
    return grpc::Status::OK;
}

template<class T>
bool DeviceServiceImpl::FetchFromMongo(mongocxx::collection& collection, const bsoncxx::document::view_or_value& filter, T* dest)
{
    bsoncxx::stdx::optional<bsoncxx::document::value> result;

    result = collection.find_one(filter.view());

    if (result) {
        mongo::BsonToMessage(result.value(), *dest);
        dest->set_uid(common::mongo::GetDocumentID(result.value().view()));
    }
    return (result.has_value());
}

bool DeviceServiceImpl::CheckIfExistInMongo(mongocxx::collection& collection, const bsoncxx::document::view_or_value& filter)
{
    auto result = collection.count_documents(filter);
    return result > 0;
}

bool DeviceServiceImpl::CreateUniqueIndex(mongocxx::collection& collection, const std::string& key)
{
    auto keys = bsoncxx::builder::basic::document{};
    keys.append(bsoncxx::builder::basic::kvp(key, 1));

    auto constraints = bsoncxx::builder::basic::document{};
    constraints.append(bsoncxx::builder::basic::kvp("unique", true));

    collection.create_index(keys.view(), constraints.view());
    return true;
}

}