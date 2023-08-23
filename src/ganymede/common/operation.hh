#ifndef GANYMEDE__COMMON__OPERATION_HH_
#define GANYMEDE__COMMON__OPERATION_HH_

#include <string>
#include <optional>

#include <grpcpp/grpcpp.h>

namespace ganymede::common {

class Void final {};

template<typename Value>
class Result {
public:
    Result() = delete;

    Result(Value&& value)
        : value_(std::move(value))
        , status_(grpc::Status::OK)
    {
    }

    Result(grpc::Status&& code)
        : status_(std::move(code))
    {
    }

    Result(grpc::StatusCode code, const std::string message)
        : status_(code, message)
    {
    }

    Result(const grpc::Status& status)
        : status_(status)
    {
    }

    operator bool() const { return status_.ok(); }

    const grpc::Status& status() const { return status_; }

    Value& mutable_value() { return *value_; }
    const Value& value() const { return value_.value(); }
    Value&& take() { return value_.value(); }

private:
    std::optional<Value> value_;
    grpc::Status status_;
};

}

#endif