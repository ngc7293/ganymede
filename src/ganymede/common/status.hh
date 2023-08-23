#ifndef GANYMEDE_COMMON_STATUS_HH_
#define GANYMEDE_COMMON_STATUS_HH_

#include <grpcpp/grpcpp.h>

namespace ganymede::common::status {

extern const grpc::Status& DATABASE_ERROR;
extern const grpc::Status& BAD_PAYLOAD;
extern const grpc::Status& NOT_FOUND;
extern const grpc::Status& UNIMPLEMENTED;
extern const grpc::Status& UNAUTHENTICATED;

}

#endif