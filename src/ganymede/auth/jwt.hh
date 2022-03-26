#ifndef GANYMEDE__AUTH__JWT_HH_
#define GANYMEDE__AUTH__JWT_HH_

#include <string>

#include <grpcpp/grpcpp.h>

namespace ganymede::auth {

bool CheckJWTTokenAndGetDomain(const grpc::ServerContext* context, std::string& domain);

}

#endif