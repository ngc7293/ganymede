#ifndef GANYMEDE__SERVICES__DEVICE__TESTS__DEVICE_SERVICE_TEST_HH_
#define GANYMEDE__SERVICES__DEVICE__TESTS__DEVICE_SERVICE_TEST_HH_

#include <gtest/gtest.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/database.hpp>

#include <ganymede/services/device/device_service_impl.hh>

class MockAuthValidator : public ganymede::auth::AuthValidator {
    ganymede::api::Result<ganymede::auth::AuthData> ValidateRequestAuth(const grpc::ServerContext&) const override
    {
        ganymede::auth::AuthData data{ "testdomain" };
        return { std::move(data) };
    }
};

class DeviceServiceTest : public ::testing::Test {
public:
    void TearDown() override
    {
        // Remove all documents from database after each test case
        mongocxx::client client{ mongocxx::uri{ "mongodb://localhost:27017/" } };
        auto devices = client["configurations"]["devices"];
        auto configs = client["configurations"]["configurations"];

        devices.delete_many(bsoncxx::builder::basic::document().extract());
        configs.delete_many(bsoncxx::builder::basic::document().extract());
    }

protected:
    ganymede::services::device::DeviceServiceImpl service{ "mongodb://localhost:27017/", std::shared_ptr<ganymede::auth::AuthValidator>(new MockAuthValidator()) };
};

std::unique_ptr<grpc::ServerContext> ServerContext()
{
    return std::make_unique<grpc::ServerContext>();
}

#endif