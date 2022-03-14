#include <memory>
#include <string>
#include <optional>

#include <grpcpp/grpcpp.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>

#include <ganymede/common/auth/jwt.hh>
#include <ganymede/common/log.hh>
#include <ganymede/common/mongo/bson_serde.hh>
#include <ganymede/common/mongo/oid.hh>
#include <ganymede/common/mongo/protobuf_collection.hh>
#include <ganymede/common/status.hh>

#include "device_service_impl.hh"

namespace {

template<class MessageType>
MessageType RemoveUIDFromMessage(const MessageType& message)
{
    MessageType result(message);

    if (result.uid() != "") {
        result.clear_uid();
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

struct DeviceServiceImpl::Priv {
    mongocxx::client client;

    mongocxx::database database;
    common::mongo::ProtobufCollection<Device> deviceCollection;
    common::mongo::ProtobufCollection<Config> configCollection;

    Priv(const std::string& mongo_uri)
        : client(mongocxx::uri{mongo_uri})
        , database(client["configurations"])
        , deviceCollection(database["device"])
        , configCollection(database["configurations"])
    {
    }
};

DeviceServiceImpl::DeviceServiceImpl(std::string mongo_uri)
    : d(new Priv(mongo_uri))
{
    d->deviceCollection.CreateUniqueIndex(Device::kMacFieldNumber);
}

DeviceServiceImpl::~DeviceServiceImpl()
{
}

grpc::Status DeviceServiceImpl::AddDevice(grpc::ServerContext* context, const AddDeviceRequest* request, Device* response)
{
    std::string domain;
    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (not request->has_device()) {
        return common::status::BAD_PAYLOAD;
    }

    Device device = RemoveUIDFromMessage(request->device());
    if (device.mac().empty() or not ValidateIsMacAddress(device.mac())) {
        return common::status::BAD_PAYLOAD;
    }

    if (not d->configCollection.Contains(request->device().config_uid(), domain)) {
        return common::status::BAD_PAYLOAD;
    }

    auto result = d->deviceCollection.CreateDocument(domain, device);
    if (result) {
        response->Swap(&result.mutable_value());
    } else {
        return result.status();
    }

    return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::GetDevice(grpc::ServerContext* context, const GetDeviceRequest* request, Device* response)
{
    std::string domain;

    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    bsoncxx::builder::basic::document filter;
    filter.append(bsoncxx::builder::basic::kvp("domain", domain));

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
    
    if (auto result = d->deviceCollection.GetDocument(filter.view())) {
        response->Swap(&result.mutable_value());
    } else {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
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
    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (not request->has_device()) {
        return common::status::BAD_PAYLOAD;
    }

    if (not d->configCollection.Contains(request->device().config_uid(), domain)) {
        return common::status::BAD_PAYLOAD;
    }

    auto result = d->deviceCollection.UpdateDocument(request->device().config_uid(), domain, RemoveUIDFromMessage(request->device()));
    if (result) {
        response->Swap(&result.mutable_value());
    } else {
        return result.status();
    }

    return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::RemoveDevice(grpc::ServerContext* context, const RemoveDeviceRequest* request, Empty* /* response */)
{
    std::string domain;
    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    return d->deviceCollection.DeleteDocument(request->device_uid(), domain).status();
}

grpc::Status DeviceServiceImpl::CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, Config* response)
{
    std::string domain;
    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (not request->has_config()){
        return common::status::BAD_PAYLOAD;
    }

    auto result = d->configCollection.CreateDocument(domain, RemoveUIDFromMessage(request->config()));
    if (result) {
        response->Swap(&result.mutable_value());
    } else {
        return result.status();
    }

    return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, Config* response)
{
    std::string domain;
    
    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (auto result = d->configCollection.GetDocument(request->config_uid(), domain)) {
        response->Swap(&result.mutable_value());
    } else {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
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
    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    if (not request->has_config()) {
        return common::status::BAD_PAYLOAD;
    }

    std::string id = request->config().uid();
    const Config& config = RemoveUIDFromMessage(request->config());

    auto result = d->configCollection.UpdateDocument(config.uid(), domain, config);
    if (result) {
        response->Swap(&result.mutable_value());
    } else {
        return result.status();
    }

    return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::DeleteConfig(grpc::ServerContext* context, const DeleteConfigRequest* request, Empty* /* response */)
{
    std::string domain;
    if (not common::auth::CheckJWTTokenAndGetDomain(context, domain)) {
        return common::status::UNAUTHENTICATED;
    }

    return d->configCollection.DeleteDocument(request->config_uid(), domain).status();
}

}