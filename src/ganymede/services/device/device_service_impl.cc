#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <date/tz.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/database.hpp>

#include <grpcpp/grpcpp.h>

#include <ganymede/api/status.hh>
#include <ganymede/log/log.hh>
#include <ganymede/mongo/oid.hh>
#include <ganymede/mongo/protobuf_collection.hh>

#include "device_service_impl.hh"

namespace {

template <class MessageType>
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

} // namespace

namespace ganymede::services::device {

struct DeviceServiceImpl::Priv {
    mongocxx::client client;

    mongocxx::database database;
    mongo::ProtobufCollection<Device> deviceCollection;
    mongo::ProtobufCollection<Config> configCollection;

    std::shared_ptr<auth::AuthValidator> authValidator;

    std::function<std::chrono::system_clock::time_point()> now;

    Priv(const std::string& mongo_uri, std::shared_ptr<auth::AuthValidator> authValidator, std::function<std::chrono::system_clock::time_point()> now)
        : client(mongocxx::uri{ mongo_uri })
        , database(client["configurations"])
        , deviceCollection(database["devices"])
        , configCollection(database["configurations"])
        , authValidator(authValidator)
        , now(now)
    {
    }
};

DeviceServiceImpl::DeviceServiceImpl(std::string mongo_uri, std::shared_ptr<auth::AuthValidator> authValidator, std::function<std::chrono::system_clock::time_point()> now)
    : d(new Priv(mongo_uri, authValidator, now))
{
    date::get_tzdb();
    d->deviceCollection.CreateUniqueIndex(Device::kMacFieldNumber);
}

DeviceServiceImpl::DeviceServiceImpl(DeviceServiceImpl&& other)
{
    d.swap(other.d);
}

DeviceServiceImpl::~DeviceServiceImpl()
{
}

Device DeviceServiceImpl::SanitizeInputDevice(const Device& input) const
{
    Device device = RemoveUIDFromMessage(input);
    device.clear_timezone_offset_minutes();
    return device;
}

api::Result<void> DeviceServiceImpl::ValidateInputDevice(const Device& input, const std::string& domain) const
{
    if (input.mac().empty() or not ValidateIsMacAddress(input.mac())) {
        return api::Result<void>(api::Status::INVALID_ARGUMENT, "missing or invalid mac address");
    }

    if (not input.timezone().empty()) {
        try {
            date::get_tzdb().locate_zone(input.timezone());
        } catch (const std::runtime_error& err) {
            return { api::Status::INVALID_ARGUMENT, "unknown timezone" };
        }
    }

    if (not d->configCollection.Contains(input.config_uid(), domain)) {
        return api::Result<void>(api::Status::NOT_FOUND, "no such config");
    }

    return {};
}

Device& DeviceServiceImpl::ComputeOutputDevice(Device& device) const
{
    if (not device.timezone().empty()) {
        const auto tz = date::get_tzdb().locate_zone(device.timezone());
        const auto offset = tz->get_info(d->now()).offset;
        device.set_timezone_offset_minutes(std::chrono::duration_cast<std::chrono::minutes>(offset).count());
    }

    return device;
}

grpc::Status DeviceServiceImpl::CreateDevice(grpc::ServerContext* context, const CreateDeviceRequest* request, Device* response)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    if (not request->has_device()) {
        return api::Result<void>(api::Status::INVALID_ARGUMENT, "empty request");
    }

    Device device = SanitizeInputDevice(request->device());
    auto validation = ValidateInputDevice(device, domain);
    if (not validation) {
        return validation;
    }

    auto result = d->deviceCollection.CreateDocument(domain, device);
    if (result) {
        response->Swap(&ComputeOutputDevice(device));
        response->set_uid(result.value());
    }

    return result;
}

grpc::Status DeviceServiceImpl::GetDevice(grpc::ServerContext* context, const GetDeviceRequest* request, Device* response)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    bsoncxx::builder::basic::document filter;
    filter.append(bsoncxx::builder::basic::kvp("domain", domain));

    switch (request->filter_case()) {
    case GetDeviceRequest::kDeviceUid:
        if (mongo::ValidateIsOid(request->device_uid())) {
            filter.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(request->device_uid())));
        } else {
            return api::Result<void>(api::Status::INVALID_ARGUMENT, "invalid device uid");
        }
        break;
    case GetDeviceRequest::kDeviceMac:
        if (ValidateIsMacAddress(request->device_mac())) {
            filter.append(bsoncxx::builder::basic::kvp(std::to_string(GetDeviceRequest::kDeviceMacFieldNumber), request->device_mac()));
        } else {
            return api::Result<void>(api::Status::INVALID_ARGUMENT, "invalid device mac");
        }
        break;
    case GetDeviceRequest::FILTER_NOT_SET:
        return api::Result<void>(api::Status::INVALID_ARGUMENT, "filter not set");
    }

    auto result = d->deviceCollection.GetDocument(filter.view());
    if (result) {
        response->Swap(&(ComputeOutputDevice(result.value())));
    }

    return result;
}

grpc::Status DeviceServiceImpl::ListDevice(grpc::ServerContext* /* context */, const ListDeviceRequest* /* request */, ListDeviceResponse* /* response */)
{
    return api::Result<void>(api::Status::UNIMPLEMENTED);
}

grpc::Status DeviceServiceImpl::UpdateDevice(grpc::ServerContext* context, const UpdateDeviceRequest* request, Device* response)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    if (not request->has_device()) {
        return api::Result<void>(api::Status::INVALID_ARGUMENT, "empty request");
    }

    Device device = SanitizeInputDevice(request->device());
    auto validation = ValidateInputDevice(device, domain);
    if (not validation) {
        return validation;
    }

    auto result = d->deviceCollection.UpdateDocument(request->device().config_uid(), domain, device);
    if (result) {
        response->Swap(&(ComputeOutputDevice(result.value())));
    }

    return result;
}

grpc::Status DeviceServiceImpl::DeleteDevice(grpc::ServerContext* context, const DeleteDeviceRequest* request, Empty* /* response */)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    return d->deviceCollection.DeleteDocument(request->device_uid(), domain);
}

grpc::Status DeviceServiceImpl::CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, Config* response)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    if (not request->has_config()) {
        return api::Result<void>(api::Status::INVALID_ARGUMENT, "empty request");
    }

    auto result = d->configCollection.CreateDocument(domain, RemoveUIDFromMessage(request->config()));
    if (result) {
        response->CopyFrom(request->config());
        response->set_uid(result.value());
    }

    return result;
}

grpc::Status DeviceServiceImpl::GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, Config* response)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    if (auto result = d->configCollection.GetDocument(request->config_uid(), domain)) {
        response->Swap(&result.value());
    } else {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
    }

    return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::ListConfig(grpc::ServerContext* /* context */, const ListConfigRequest* /* request */, ListConfigResponse* /* response */)
{
    return api::Result<void>(api::Status::UNIMPLEMENTED);
}

grpc::Status DeviceServiceImpl::UpdateConfig(grpc::ServerContext* context, const UpdateConfigRequest* request, Config* response)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    if (not request->has_config()) {
        return api::Result<void>(api::Status::INVALID_ARGUMENT, "empty request");
    }

    std::string id = request->config().uid();
    const Config& config = RemoveUIDFromMessage(request->config());

    auto result = d->configCollection.UpdateDocument(config.uid(), domain, config);
    if (result) {
        response->Swap(&result.value());
    }

    return result;
}

grpc::Status DeviceServiceImpl::DeleteConfig(grpc::ServerContext* context, const DeleteConfigRequest* request, Empty* /* response */)
{
    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        return auth;
    }

    const std::string& domain = auth.value().domain;

    return d->configCollection.DeleteDocument(request->config_uid(), domain);
}

} // namespace ganymede::services::device