#ifndef GANYMEDE__SERVICES__DEVICE__DEVICE_SERVICE_IMPL_HH_
#define GANYMEDE__SERVICES__DEVICE__DEVICE_SERVICE_IMPL_HH_

#include <optional>

#include <grpcpp/grpcpp.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>

#include "device.config.pb.h"
#include "device.grpc.pb.h"
#include "device.pb.h"

namespace ganymede::services::device {

class DeviceServiceImpl final : public DeviceService::Service {
public:
    DeviceServiceImpl() = delete;
    DeviceServiceImpl(std::string mongo_uri);
    ~DeviceServiceImpl();

    grpc::Status AddDevice(grpc::ServerContext* context, const AddDeviceRequest* request, Device* response) override;
    grpc::Status GetDevice(grpc::ServerContext* context, const GetDeviceRequest* request, Device* response) override;
    grpc::Status ListDevice(grpc::ServerContext* context, const ListDeviceRequest* request, ListDeviceResponse* response) override;
    grpc::Status UpdateDevice(grpc::ServerContext* context, const UpdateDeviceRequest* request, Device* response) override;
    grpc::Status RemoveDevice(grpc::ServerContext* context, const RemoveDeviceRequest* request, Empty* response) override;

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