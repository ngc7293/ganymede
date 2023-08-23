#ifndef GANYMEDE__API__RESULT_HH_
#define GANYMEDE__API__RESULT_HH_

#include <optional>
#include <string>

#include <ganymede/api/status.hh>

namespace ganymede::api {

template <typename Value>
class Result {
public:
    Result() = delete;
    Result(const Value& value) = delete;

    Result(Value&& value)
        : value_(std::move(value))
        , status_(Status::OK)
    {
    }

    Result(api::Status status, std::string&& error)
        : status_(status)
        , error_(std::move(error))
    {
    }

    Result(api::Status status, const std::string& error = "")
        : status_(status)
        , error_(error)
    {
    }

    operator bool() const { return status_ == api::Status::OK; }
    operator grpc::Status() const { return grpc::Status(static_cast<grpc::StatusCode>(status_), error_); }

    api::Status status() const { return status_; }
    const std::string& error() const { return error_; }

    Value& value() & { return *value_; }
    const Value& value() const& { return value_.value(); }

    Value&& value() && { return *value_; }
    const Value&& value() const&& { return value_.value(); }

private:
    friend class Result<void>;

    std::optional<Value> value_;
    api::Status status_;
    std::string error_;
};

template <>
class Result<void> {
public:
    Result()
        : status_(api::Status::OK)
    {
    }

    template <typename T>
    Result(Result<T>&& other)
        : status_(other.status_)
        , error_(other.error_)
    {
    }

    Result(api::Status status, std::string&& error)
        : status_(status)
        , error_(std::move(error))
    {
    }

    Result(api::Status status, const std::string& error = "")
        : status_(status)
        , error_(error)
    {
    }

    operator bool() const { return status_ == api::Status::OK; }
    operator grpc::Status() const { return grpc::Status(static_cast<grpc::StatusCode>(status_), error_); }

    api::Status status() const { return status_; }
    const std::string& error() const { return error_; }

private:
    api::Status status_;
    std::string error_;
};

} // namespace ganymede::api

#endif