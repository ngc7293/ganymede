#include <grpc/status.h>
#include <grpcpp/grpcpp.h>

#include <ganymede/common/status.hh>

namespace ganymede::common::status {

const grpc::Status& DATABASE_ERROR = grpc::Status(grpc::StatusCode::UNKNOWN, "database error");
const grpc::Status& BAD_PAYLOAD = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "bad payload");
const grpc::Status& NOT_FOUND = grpc::Status(grpc::StatusCode::NOT_FOUND, "no such resource");
const grpc::Status& UNIMPLEMENTED = grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet implemented");
const grpc::Status& UNAUTHENTICATED = grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "authentification error");

}