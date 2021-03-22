#ifndef GANYMEDE_COMMON_STATUS_HH_
#define GANYMEDE_COMMON_STATUS_HH_

#include <grpc/status.h>

namespace ganymede::common::status {

const grpc::Status& DATABASE_ERROR = grpc::Status(grpc::StatusCode::UNKNOWN, "database error");
const grpc::Status& BAD_PAYLOAD = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "bad payload");
const grpc::Status& UNIMPLEMENTED = grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet implemented");

}

#endif