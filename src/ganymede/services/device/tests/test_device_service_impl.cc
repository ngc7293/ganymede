// clang-format off

#include <gtest/gtest.h>

#include <date/date.h>

#include <nlohmann/json.hpp>

#include "device_service_test.hh"

using namespace date::literals;

TEST_F(DeviceServiceTest, should_add_config)
{

    nlohmann::json request = {
        { "config", {
            { "uid", "alphabravo" },
            { "display_name", "alphabravo" },
            { "light_config", {
                { "luminaires", {
                    {
                        { "port", 1 },
                        { "use_pwm", true }
                    }
                }}
            }}
        }}
    };

    auto response = Call(&ganymede::services::device::DeviceServiceImpl::CreateConfig, request).value();
    EXPECT_NE(response["uid"], "");
    EXPECT_NE(response["uid"], "alphabravo");
    EXPECT_EQ(response["display_name"], "alphabravo");
}

TEST_F(DeviceServiceTest, should_add_device)
{
    nlohmann::json request = {
        { "device", {
            { "uid", "charliedelta" },
            { "display_name", "my device" },
            { "mac", "00:00:00:00:00:00" },
            { "config_uid", MakeConfig() }
        }}
    };

    auto response = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request).value();
    EXPECT_NE(response["uid"], "");
    EXPECT_NE(response["uid"], "charliedelta");
    EXPECT_EQ(response["display_name"], "my device");
    EXPECT_EQ(response["mac"], "00:00:00:00:00:00");
}

TEST_F(DeviceServiceTest, should_get_device)
{
    nlohmann::json create_request = {
        { "device", {
            { "uid", "charliedelta" },
            { "display_name", "my device" },
            { "mac", "00:00:00:00:00:00" },
            { "config_uid", MakeConfig() }
        }}
    };

    auto create_result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, create_request).value();

    auto response = Call(&ganymede::services::device::DeviceServiceImpl::GetDevice, {{ "device_uid", create_result["uid"] }}).value();
    EXPECT_EQ(response["uid"], create_result["uid"]);
    EXPECT_EQ(response["display_name"], "my device");
    EXPECT_EQ(response["mac"], "00:00:00:00:00:00");
}

TEST_F(DeviceServiceTest, should_refuse_invalid_timezone)
{
    nlohmann::json request = {
        { "device", {
            { "mac", "00:00:00:00:00:00" },
            { "config_uid", MakeConfig() },
            { "timezone", "Rohan/Edoras" }
        }}
    };

    {
        auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request, false);
        EXPECT_EQ(result.status(), ganymede::api::Status::INVALID_ARGUMENT);
    }
    {
        request["device"]["uid"] = MakeDevice();
        request["device"]["timezone"] = "Rohan/Edoras";
        auto result = Call(&ganymede::services::device::DeviceServiceImpl::UpdateDevice, request, false);
        EXPECT_EQ(result.status(), ganymede::api::Status::INVALID_ARGUMENT);
    }
}

TEST_F(DeviceServiceTest, should_return_timezone_offset)
{
    now = []() { return static_cast<date::sys_days>(2022_y / date::January / 1); };\

    {
        // Check on create
        nlohmann::json request = {
            { "device", {
                { "mac", "00:00:00:00:00:00" },
                { "config_uid", MakeConfig() },
                { "timezone", "America/Montreal" }
            }}
        };

        auto response = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request).value();
        EXPECT_EQ(response["timezone_offset_minutes"], "-300");
    }
    {
        // Check on Get
        nlohmann::json request = {
            { "device", {
                { "mac", "00:00:00:00:00:01" },
                { "config_uid", MakeConfig() },
                { "timezone", "Europe/Berlin" }
            }}
        };

        auto create_response = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request).value();
        EXPECT_EQ(create_response["timezone_offset_minutes"], "60");

        auto get_response = Call(&ganymede::services::device::DeviceServiceImpl::GetDevice, {{ "device_mac", "00:00:00:00:00:01" }}).value();
        EXPECT_EQ(get_response["timezone_offset_minutes"], "60");
    }
    {
        // Check on Update
        nlohmann::json update_request = {
            { "device", {
                { "uid", MakeDevice("00:00:00:00:00:02") },
                { "mac", "00:00:00:00:00:02"},
                { "config_uid", MakeConfig() },
                { "timezone", "Asia/Shanghai" }
            }}
        };

        auto update_response = Call(&ganymede::services::device::DeviceServiceImpl::UpdateDevice, update_request).value();
        EXPECT_EQ(update_response["timezone_offset_minutes"], "480");
    }
}

TEST_F(DeviceServiceTest, should_refuse_add_device_if_no_such_config)
{
    {
        nlohmann::json request = {
            { "device", {
                { "mac", "00:00:00:00:00:00"},
                { "config_uid", "wigwam" },
            }}
        };

        auto result = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, request, false);
        EXPECT_EQ(result.status(), ganymede::api::Status::NOT_FOUND);
        EXPECT_EQ(result.error(), "no such config");
    }
    {
        nlohmann::json request = {
            { "device", {
                { "uid", MakeDevice("00:00:00:00:00:01") },
                { "mac", "00:00:00:00:00:01"},
                { "config_uid", "wigwam" }
            }}
        };

        auto result = Call(&ganymede::services::device::DeviceServiceImpl::UpdateDevice, request, false);
        EXPECT_EQ(result.status(), ganymede::api::Status::NOT_FOUND);
        EXPECT_EQ(result.error(), "no such config");
    }
}

TEST_F(DeviceServiceTest, should_update_device)
{
    nlohmann::json create_request = {
        { "device", {
            { "mac", "00:00:00:00:00:01" },
            { "display_name", "Alpha" },
            { "config_uid", MakeConfig() },
            { "timezone", "Europe/Berlin" }
        }}
    };

    auto create_response = Call(&ganymede::services::device::DeviceServiceImpl::CreateDevice, create_request).value();
    
    nlohmann::json update_request = {
        { "device", {
            { "uid", create_response["uid"] },
            { "mac", "ff:00:ff:00:ff:00" },
            { "display_name", "Bravo" },
            { "config_uid", MakeConfig() },
            { "timezone", "America/Toronto" }
        }}
    };

    auto response = Call(&ganymede::services::device::DeviceServiceImpl::UpdateDevice, update_request).value();
    EXPECT_EQ(response["uid"], create_response["uid"]);
    EXPECT_NE(response["config_uid"], create_response["config_uid"]);

    EXPECT_EQ(response["mac"], "ff:00:ff:00:ff:00");
    EXPECT_EQ(response["display_name"], "Bravo");
    EXPECT_EQ(response["timezone"], "America/Toronto");
}
