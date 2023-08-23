#ifndef GANYMEDE__TESTING__SERVICE_TEST_HH_
#define GANYMEDE__TESTING__SERVICE_TEST_HH_

#include <gtest/gtest.h>

#include <google/protobuf/util/json_util.h>

#include <grpcpp/grpcpp.h>

#include <nlohmann/json.hpp>

namespace ganymede::testing {

template <class Service>
class ServiceTest : public ::testing::Test {
protected:
    ServiceTest(Service&& service)
        : service(std::move(service))
    {
    }

    template <class Response, class Request>
    ganymede::api::Result<nlohmann::json> Call(grpc::Status (Service::*rpc)(grpc::ServerContext*, const Request*, Response*), const nlohmann::json& request, bool expect = true)
    {
        auto result = Call(rpc, ToProto<Request>(request), expect);

        if (result) {
            return { ToJson(result.value()) };
        } else {
            return { result.status(), result.error() };
        }
    }

    template <class Response, class Request>
    ganymede::api::Result<Response> Call(grpc::Status (Service::*rpc)(grpc::ServerContext*, const Request*, Response*), const Request& request, bool expect = true)
    {
        Response response;
        auto result = (service.*rpc)(std::make_unique<grpc::ServerContext>().get(), &request, &response);
        EXPECT_EQ(result.ok(), expect) << "[" << result.error_code() << "] " << result.error_message() << " : " << result.error_details() << std::endl;

        if (result.ok()) {
            return { std::move(response) };
        } else {
            return { static_cast<ganymede::api::Status>(result.error_code()), result.error_message() };
        }
    }

private:
    template <class Message>
    Message ToProto(const nlohmann::json& json)
    {
        Message message;
        google::protobuf::util::JsonStringToMessage(json.dump(), &message);
        return message;
    }

    template <class Message>
    nlohmann::json ToJson(const Message& proto)
    {
        static google::protobuf::util::JsonPrintOptions options;
        options.preserve_proto_field_names = true;

        std::string json;
        google::protobuf::util::MessageToJsonString(proto, &json, options);
        return nlohmann::json::parse(json);
    }

protected:
    Service service;
};

} // namespace ganymede::testing

#endif