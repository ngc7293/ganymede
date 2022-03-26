#ifndef GANYMEDE__API__RESULT_HH_
#define GANYMEDE__API__RESULT_HH_

#include <string>
#include <optional>

#include <ganymede/api/status.hh>

namespace ganymede::api {

template<typename Value>
class Result {
public:
    Result() = delete;

    Result(Value&& value) : value_(std::move(value)), status_(Status::OK) { }
    Result(api::Status status, std::string&& error) : status_(status), error_(std::move(error)) { }
    Result(api::Status status, const std::string& error = "") : status_(status), error_(error) { }

    operator bool() const { return status_ == api::Status::OK; }
    operator grpc::Status() const { return grpc::Status(static_cast<grpc::StatusCode>(status_), error_); }

    api::Status status() const { return status_; }
    const std::string& error() const { return error_; }

    Value& value() & { return *value_; }
    const Value& value() const& { return value_.value(); }

    Value&& value() && { return *value_; }
    const Value&& value() const&& { return value_.value(); }

private:
    std::optional<Value> value_;
    api::Status status_;
    std::string error_;
};

template<>
class Result<void> {
public:
    Result() : status_(api::Status::OK) { }
    Result(api::Status status, std::string&& error) : status_(status), error_(std::move(error)) { }
    Result(api::Status status, const std::string& error = "") : status_(status), error_(error) { }

    operator bool() const { return status_ == api::Status::OK; }
    operator grpc::Status() const { return grpc::Status(static_cast<grpc::StatusCode>(status_), error_); }

    api::Status status() const { return status_; }
    const std::string& error() const { return error_; }

private:
    api::Status status_;
    std::string error_;
};


}

#endif