#ifndef GANYMEDE__TESTING__SERVICE_TEST_HH_
#define GANYMEDE__TESTING__SERVICE_TEST_HH_

#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>

namespace ganymede::testing {

template <class Service>
class ServiceTest : public ::testing::Test {
protected:
    ServiceTest(Service&& service)
        : service(std::move(service))
    {
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

protected:
    Service service;
};

} // namespace ganymede::testing

#endif