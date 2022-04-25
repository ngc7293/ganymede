#include <gtest/gtest.h>

#include "device_service_test.hh"

TEST_F(DeviceServiceTest, should_add_config)
{
    ganymede::services::device::CreateConfigRequest request;
    ganymede::services::device::Config response;
    request.mutable_config()->set_uid("alphabravo");
    request.mutable_config()->set_display_name("alphabravo");
    auto luminaire = request.mutable_config()->mutable_light_config()->add_luminaires();
    luminaire->set_port(1);
    luminaire->set_use_pwm(true);

    auto result = service.CreateConfig(ServerContext().get(), &request, &response);
    ASSERT_TRUE(result.ok()) << "[" << result.error_code() << "] " << result.error_message() << " : " << result.error_details() << std::endl;
    EXPECT_NE(response.uid(), request.config().uid());
    EXPECT_EQ(response.display_name(), request.config().display_name());
}

TEST_F(DeviceServiceTest, should_add_device)
{
    std::string config_uid;

    {
        ganymede::services::device::CreateConfigRequest request;
        ganymede::services::device::Config response;

        (void)request.mutable_config();

        auto result = service.CreateConfig(ServerContext().get(), &request, &response);
        ASSERT_TRUE(result.ok()) << "[" << result.error_code() << "] " << result.error_message() << " : " << result.error_details() << std::endl;
        config_uid = response.uid();
    }

    {
        ganymede::services::device::AddDeviceRequest request;
        ganymede::services::device::Device response;

        request.mutable_device()->set_uid("charliedelta");
        request.mutable_device()->set_config_uid(config_uid);
        request.mutable_device()->set_name("my device");
        request.mutable_device()->set_mac("00:00:00:00:00:00");

        auto result = service.AddDevice(ServerContext().get(), &request, &response);
        ASSERT_TRUE(result.ok()) << "[" << result.error_code() << "] " << result.error_message() << " : " << result.error_details() << std::endl;

        EXPECT_NE(response.uid(), request.device().uid());
        EXPECT_EQ(response.config_uid(), config_uid);
        EXPECT_EQ(response.name(), request.device().name());
        EXPECT_EQ(response.mac(), request.device().mac());
    }
}

TEST_F(DeviceServiceTest, should_refuse_add_device_if_no_such_config)
{
    ganymede::services::device::AddDeviceRequest request;
    ganymede::services::device::Device response;

    request.mutable_device()->set_config_uid("wigwam");
    request.mutable_device()->set_mac("00:00:00:00:00:00");

    auto result = service.AddDevice(ServerContext().get(), &request, &response);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error_code(), grpc::StatusCode::NOT_FOUND);
}