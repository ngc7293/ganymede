#ifndef GANYMEDE__AUTH__AUTH_VALIDATOR_HH_
#define GANYMEDE__AUTH__AUTH_VALIDATOR_HH_

#include <grpcpp/grpcpp.h>

#include <ganymede/api/result.hh>

namespace ganymede::auth {

struct AuthData {
    std::string domain;
};

class AuthValidator {
public:
    virtual api::Result<AuthData> ValidateRequestAuth(const grpc::ServerContext& context) const = 0;
};

class JwtAuthValidator : public AuthValidator {
public:
    api::Result<AuthData> ValidateRequestAuth(const grpc::ServerContext& context) const override;
};

} // namespace ganymede::auth

#endif