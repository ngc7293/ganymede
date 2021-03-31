#ifndef GANYMEDE_COMMON_AUTH_JWT_HH_
#define GANYMEDE_COMMON_AUTH_JWT_HH_

#include <string>

#include <grpcpp/grpcpp.h>

namespace ganymede::common::auth {

bool CheckJWTTokenAndGetDomain(const grpc::ServerContext* context, std::string& domain);

}

#endif