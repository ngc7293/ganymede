#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <date/tz.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/pool.hpp>

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
    mongocxx::pool pool;

    std::shared_ptr<auth::AuthValidator> authValidator;

    std::function<std::chrono::system_clock::time_point()> now;

    Priv(const std::string& mongo_uri, std::shared_ptr<auth::AuthValidator> authValidator, std::function<std::chrono::system_clock::time_point()> now)
        : pool(mongocxx::uri{ mongo_uri })
        , authValidator(authValidator)
        , now(now)
    {
    }

    mongo::ProtobufCollection<Device> Devices(mongocxx::client& client)
    {
        return mongo::ProtobufCollection<Device>(client[DATABASE_NAME][DEVICE_COLLECTION_NAME]);
    }

    mongo::ProtobufCollection<Config> Configs(mongocxx::client& client)
    {
        return mongo::ProtobufCollection<Config>(client[DATABASE_NAME][CONFIG_COLLECTION_NAME]);
    }

    api::Result<void> ValidateInputDevice(const Device& input, const std::string& domain, mongocxx::client& client)
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

        if (not Configs(client).Contains(input.config_uid(), domain)) {
            return api::Result<void>(api::Status::NOT_FOUND, "no such config");
        }

        return {};
    }

    Device SanitizeInputDevice(const Device& input) const
    {
        Device device = RemoveUIDFromMessage(input);
        device.clear_timezone_offset_minutes();
        return device;
    }

    Device& ComputeOutputDevice(Device& device) const
    {
        if (not device.timezone().empty()) {
            const auto tz = date::get_tzdb().locate_zone(device.timezone());
            const auto offset = tz->get_info(now()).offset;
            device.set_timezone_offset_minutes(std::chrono::duration_cast<std::chrono::minutes>(offset).count());
        }

        return device;
    }
};

DeviceServiceImpl::DeviceServiceImpl(std::string mongo_uri, std::shared_ptr<auth::AuthValidator> authValidator, std::function<std::chrono::system_clock::time_point()> now)
    : d(new Priv(mongo_uri, authValidator, now))
{
    date::get_tzdb();

    auto client = d->pool.acquire();
    d->Devices(*client).CreateUniqueIndex(Device::kMacFieldNumber);
}

DeviceServiceImpl::DeviceServiceImpl(DeviceServiceImpl&& other)
{
    d.swap(other.d);
}

DeviceServiceImpl::~DeviceServiceImpl()
{
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

    auto client = d->pool.acquire();
    Device device = d->SanitizeInputDevice(request->device());

    auto validation = d->ValidateInputDevice(device, domain, *client);
    if (not validation) {
        return validation;
    }

    auto result = d->Devices(*client).CreateDocument(domain, device);
    if (result) {
        response->Swap(&(d->ComputeOutputDevice(device)));
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
            filter.append(bsoncxx::builder::basic::kvp(std::to_string(Device::kMacFieldNumber), request->device_mac()));
        } else {
            return api::Result<void>(api::Status::INVALID_ARGUMENT, "invalid device mac");
        }
        break;
    case GetDeviceRequest::FILTER_NOT_SET:
        return api::Result<void>(api::Status::INVALID_ARGUMENT, "filter not set");
    }

    auto client = d->pool.acquire();
    auto result = d->Devices(*client).GetDocument(filter.view());
    if (result) {
        response->Swap(&(d->ComputeOutputDevice(result.value())));
    }

    return result;
}

grpc::Status DeviceServiceImpl::ListDevice(grpc::ServerContext* context, const ListDeviceRequest* request, ListDeviceResponse* response)
{
    log::info("request started", { { "context", reinterpret_cast<std::uint64_t>(context) } });

    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        log::info("request failed", { { "context", reinterpret_cast<std::uint64_t>(context) } });
        return auth;
    }

    const std::string& domain = auth.value().domain;
    auto client = d->pool.acquire();

    bsoncxx::builder::basic::document filter;
    filter.append(bsoncxx::builder::basic::kvp("domain", domain));

    switch (request->filter_case()) {
    case ListDeviceRequest::kNameFilter: {
        bsoncxx::builder::basic::document name_filter;
        name_filter.append(bsoncxx::builder::basic::kvp(std::string("$regex"), request->name_filter()));
        filter.append(bsoncxx::builder::basic::kvp(std::to_string(Device::kDisplayNameFieldNumber), name_filter.extract()));
    } break;
    case ListDeviceRequest::kConfigUid: {
        if (d->Configs(*client).Contains(request->config_uid(), domain)) {
            filter.append(bsoncxx::builder::basic::kvp(std::to_string(Device::kConfigUidFieldNumber), request->config_uid()));
        } else {
            log::info("request failed", { { "context", reinterpret_cast<std::uint64_t>(context) } });
            return api::Result<void>(api::Status::NOT_FOUND, "no such config");
        }
    } break;
    case ListDeviceRequest::FILTER_NOT_SET:
        break;
    }

    auto result = d->Devices(*client).ListDocuments(filter.view());
    if (result) {
        for (auto& device : result.value()) {
            response->add_devices()->Swap(&(d->ComputeOutputDevice(device)));
        }
    }

    log::info("request succeeded", { { "context", reinterpret_cast<std::uint64_t>(context) } });
    return result;
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

    auto client = d->pool.acquire();
    const std::string& id = request->device().uid();
    const Device device = d->SanitizeInputDevice(request->device());

    auto validation = d->ValidateInputDevice(device, domain, *client);
    if (not validation) {
        return validation;
    }

    auto result = d->Devices(*client).UpdateDocument(id, domain, device);
    if (result) {
        response->Swap(&(d->ComputeOutputDevice(result.value())));
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

    auto client = d->pool.acquire();
    return d->Devices(*client).DeleteDocument(request->device_uid(), domain);
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

    auto client = d->pool.acquire();
    auto result = d->Configs(*client).CreateDocument(domain, RemoveUIDFromMessage(request->config()));
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

    auto client = d->pool.acquire();
    if (auto result = d->Configs(*client).GetDocument(request->config_uid(), domain)) {
        response->Swap(&result.value());
    } else {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
    }

    return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::ListConfig(grpc::ServerContext* context, const ListConfigRequest* request, ListConfigResponse* response)
{
    log::info("request started", { { "context", reinterpret_cast<std::uint64_t>(context) } });

    auto auth = d->authValidator->ValidateRequestAuth(*context);
    if (not auth) {
        log::info("request failed", { { "context", reinterpret_cast<std::uint64_t>(context) } });
        return auth;
    }

    const std::string& domain = auth.value().domain;
    auto client = d->pool.acquire();

    bsoncxx::builder::basic::document filter;
    filter.append(bsoncxx::builder::basic::kvp("domain", domain));

    if (not request->name_filter().empty()) {
        bsoncxx::builder::basic::document name_filter;
        name_filter.append(bsoncxx::builder::basic::kvp(std::string("$regex"), request->name_filter()));
        filter.append(bsoncxx::builder::basic::kvp(std::to_string(Config::kDisplayNameFieldNumber), name_filter.extract()));
    }

    auto result = d->Configs(*client).ListDocuments(filter.view());
    if (result) {
        for (auto& config : result.value()) {
            response->add_configs()->Swap(&config);
        }
    }

    log::info("request succeeded", { { "context", reinterpret_cast<std::uint64_t>(context) } });
    return result;
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

    auto client = d->pool.acquire();
    auto result = d->Configs(*client).UpdateDocument(id, domain, config);
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

    auto client = d->pool.acquire();
    return d->Configs(*client).DeleteDocument(request->config_uid(), domain);
}

} // namespace ganymede::services::device