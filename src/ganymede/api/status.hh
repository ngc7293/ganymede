#ifndef GANYMEDE__API__STATUS_HH_
#define GANYMEDE__API__STATUS_HH_

#include <grpcpp/grpcpp.h>

namespace ganymede::api {

enum class Status {
    OK = grpc::StatusCode::OK,
    INTERNAL = grpc::StatusCode::INTERNAL,
    INVALID_ARGUMENT = grpc::StatusCode::INVALID_ARGUMENT,
    NOT_FOUND = grpc::StatusCode::NOT_FOUND,
    UNAUTHENTICATED = grpc::StatusCode::UNAUTHENTICATED,
    UNIMPLEMENTED = grpc::StatusCode::UNIMPLEMENTED
};

}

#endif