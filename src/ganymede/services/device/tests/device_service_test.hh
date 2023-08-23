#ifndef GANYMEDE__SERVICES__DEVICE__TESTS__DEVICE_SERVICE_TEST_HH_
#define GANYMEDE__SERVICES__DEVICE__TESTS__DEVICE_SERVICE_TEST_HH_

#include <gtest/gtest.h>

#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/database.hpp>

#include <ganymede/services/device/device_service_impl.hh>
#include <ganymede/testing/mock_auth_validator.hh>
#include <ganymede/testing/service_test.hh>

class DeviceServiceTest : public ganymede::testing::ServiceTest<ganymede::services::device::DeviceServiceImpl> {
public:
    DeviceServiceTest()
        : ServiceTest({ "mongodb://localhost:27017/", std::shared_ptr<ganymede::auth::AuthValidator>(new ganymede::testing::MockAuthValidator()), [this]() { return now(); } })
    {
    }

    std::string MakeConfig()
    {
        ganymede::services::device::CreateConfigRequest request;
        (void)request.mutable_config();

        auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateConfig, request);
        return result.value().uid();
    }

    std::string MakeDevice(const std::string& mac = "00:00:00:00:00:00")
    {
        ganymede::services::device::CreateDeviceRequest request;
        request.mutable_device()->set_mac(mac);
        request.mutable_device()->set_config_uid(MakeConfig());

        auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request);
        return result.value().uid();
    }

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
    std::function<std::chrono::system_clock::time_point()> now{ std::chrono::system_clock::now };
};

#endif