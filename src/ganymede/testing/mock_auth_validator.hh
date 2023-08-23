#ifndef GANYMEDE__TESTING__MOCK_AUTH_VALIDATOR_HH_
#define GANYMEDE__TESTING__MOCK_AUTH_VALIDATOR_HH_

#include <ganymede/auth/auth_validator.hh>

namespace ganymede::testing {

class MockAuthValidator : public ganymede::auth::AuthValidator {
    ganymede::api::Result<ganymede::auth::AuthData> ValidateRequestAuth(const grpc::ServerContext&) const override
    {
        ganymede::auth::AuthData data{ "testdomain" };
        return { std::move(data) };
    }
};

} // namespace ganymede::testing

#endif