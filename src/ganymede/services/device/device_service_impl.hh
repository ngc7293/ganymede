#ifndef GANYMEDE__SERVICES__DEVICE__DEVICE_SERVICE_IMPL_HH_
#define GANYMEDE__SERVICES__DEVICE__DEVICE_SERVICE_IMPL_HH_

#include <grpcpp/grpcpp.h>

#include <ganymede/auth/auth_validator.hh>

#include "device.config.pb.h"
#include "device.grpc.pb.h"
#include "device.pb.h"

namespace ganymede::services::device {

class DeviceServiceImpl final : public DeviceService::Service {
public:
    DeviceServiceImpl() = delete;
    DeviceServiceImpl(DeviceServiceImpl&& other); // Used in testing
    DeviceServiceImpl(std::string mongo_uri, std::shared_ptr<auth::AuthValidator> authValidator, std::function<std::chrono::system_clock::time_point()> now = std::chrono::system_clock::now);
    ~DeviceServiceImpl();

    Device SanitizeInputDevice(const Device& input) const;
    api::Result<void> ValidateInputDevice(const Device& input, const std::string& domain) const;
    Device& ComputeOutputDevice(Device& device) const;

    grpc::Status CreateDevice(grpc::ServerContext* context, const CreateDeviceRequest* request, Device* response) override;
    grpc::Status GetDevice(grpc::ServerContext* context, const GetDeviceRequest* request, Device* response) override;
    grpc::Status ListDevice(grpc::ServerContext* context, const ListDeviceRequest* request, ListDeviceResponse* response) override;
    grpc::Status UpdateDevice(grpc::ServerContext* context, const UpdateDeviceRequest* request, Device* response) override;
    grpc::Status DeleteDevice(grpc::ServerContext* context, const DeleteDeviceRequest* request, Empty* response) override;

    grpc::Status CreateConfig(grpc::ServerContext* context, const CreateConfigRequest* request, Config* response) override;
    grpc::Status GetConfig(grpc::ServerContext* context, const GetConfigRequest* request, Config* response) override;
    grpc::Status ListConfig(grpc::ServerContext* context, const ListConfigRequest* request, ListConfigResponse* response) override;
    grpc::Status UpdateConfig(grpc::ServerContext* context, const UpdateConfigRequest* request, Config* response) override;
    grpc::Status DeleteConfig(grpc::ServerContext* context, const DeleteConfigRequest* request, Empty* response) override;

private:
    struct Priv;
    std::unique_ptr<Priv> d;
};

} // namespace ganymede::services::device

#endif