#ifndef GANYMEDE__SERVICES__DEVICE__DEVICE_SERVICE_IMPL_HH_
#define GANYMEDE__SERVICES__DEVICE__DEVICE_SERVICE_IMPL_HH_

#include <optional>

#include <grpcpp/grpcpp.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>

#include "device.pb.h"
#include "device.grpc.pb.h"
#include "device.config.pb.h"

namespace ganymede::services::device {

class DeviceServiceImpl final : public DeviceService::Service {
public:
    DeviceServiceImpl() = delete;
    DeviceServiceImpl(std::string mongo_uri);

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
    std::optional<grpc::Status> TryInsertDocumentIntoMongo(mongocxx::collection& collection, const bsoncxx::builder::basic::document& builder, std::string& id);
    std::optional<grpc::Status> TryInsertDocumentIntoMongo(mongocxx::collection& collection, const bsoncxx::builder::basic::document& builder, std::string& id, int& ec);

    template<class DocumentType>
    bool FetchFromMongo(mongocxx::collection& collection, const bsoncxx::document::view_or_value& filter, DocumentType* dest);
    bool CheckIfExistInMongo(mongocxx::collection& collection, const bsoncxx::document::view_or_value& filter);
    bool CreateUniqueIndex(mongocxx::collection& collection, const std::string& key);

private:
    mongocxx::client m_mongoclient;
    mongocxx::database m_mongodb;
    mongocxx::collection m_device_collection;
    mongocxx::collection m_config_collection;
};

}

#endif