#include <gtest/gtest.h>

#include <date/date.h>

#include "device_service_test.hh"

using namespace date::literals;

TEST_F(DeviceServiceTest, should_add_config)
{
    ganymede::services::device::CreateConfigRequest request;
    request.mutable_config()->set_uid("alphabravo");
    request.mutable_config()->set_display_name("alphabravo");
    auto luminaire = request.mutable_config()->mutable_light_config()->add_luminaires();
    luminaire->set_port(1);
    luminaire->set_use_pwm(true);

    auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateConfig, request);
    EXPECT_NE(result.value().uid(), request.config().uid());
    EXPECT_EQ(result.value().display_name(), request.config().display_name());
}

TEST_F(DeviceServiceTest, should_add_device)
{
    ganymede::services::device::CreateDeviceRequest request;

    request.mutable_device()->set_uid("charliedelta");
    request.mutable_device()->set_config_uid(MakeConfig());
    request.mutable_device()->set_display_name("my device");
    request.mutable_device()->set_mac("00:00:00:00:00:00");

    auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request);
    EXPECT_NE(result.value().uid(), request.device().uid());
    EXPECT_EQ(result.value().display_name(), request.device().display_name());
    EXPECT_EQ(result.value().mac(), request.device().mac());
}

TEST_F(DeviceServiceTest, should_refuse_invalid_timezone)
{
    ganymede::services::device::CreateDeviceRequest request;

    request.mutable_device()->set_config_uid(MakeConfig());
    request.mutable_device()->set_mac("00:00:00:00:00:00");
    request.mutable_device()->set_timezone("Rohan/Edoras");

    auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request, false);
    EXPECT_EQ(result.status(), ganymede::api::Status::INVALID_ARGUMENT);
}

TEST_F(DeviceServiceTest, should_return_timezone_offset)
{
    now = []() { return static_cast<date::sys_days>(2022_y / date::January / 1); };

    {
        ganymede::services::device::CreateDeviceRequest request;

        request.mutable_device()->set_config_uid(MakeConfig());
        request.mutable_device()->set_mac("00:00:00:00:00:00");
        request.mutable_device()->set_timezone("America/Montreal");

        auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request);
        EXPECT_EQ(result.value().timezone_offset_minutes(), -300);
    }
    {
        ganymede::services::device::CreateDeviceRequest request;

        request.mutable_device()->set_config_uid(MakeConfig());
        request.mutable_device()->set_mac("00:00:00:00:00:01");
        request.mutable_device()->set_timezone("Europe/Berlin");

        auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request);
        EXPECT_EQ(result.value().timezone_offset_minutes(), 60);
    }
}

TEST_F(DeviceServiceTest, should_refuse_add_device_if_no_such_config)
{
    ganymede::services::device::CreateDeviceRequest request;

    request.mutable_device()->set_config_uid("wigwam");
    request.mutable_device()->set_mac("00:00:00:00:00:00");

    auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request, false);
    EXPECT_EQ(result.status(), ganymede::api::Status::NOT_FOUND);
}