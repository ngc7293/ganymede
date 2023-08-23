#include <jwt/jwt.hpp>

#include "auth_validator.hh"

namespace ganymede::auth {

const char* AUTH0_PUB_KEY = R"(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArV/vHT8ATpXy3Ns/KARs
P+T4In5Ex9k5iAXjkIPlT1+pzm2P5yYLIeGQ+X9jtQ3AGpL7E0xgX96QpIzL7AI5
uSIdcyHOpiGMdgfIknH+Gmg0HCyCsoYflr7L0S0lRSJpEGCKp9WaV1JexLEgZREf
vPz/j5yKR/uAJf+QlhcHgl0/GmxjRgg8Aex3/dSCjsOmiQ62EDTOAfXeu5oec2n+
TFHaVhH3EEbVMXzyAiL6QZKDSvTIqRO2t/M5ogQB7JQko5wVQanlCvAnMHTaUvbQ
4aEu9tyMLqcdx+MKys1Hv1zHw6rA212fTj4hHWnYciA31f9sljjWM5eqAvzro+Q9
BwIDAQAB
-----END PUBLIC KEY-----
)";

const char* TOKEN_ISSUER = "https://dev-ganymede.us.auth0.com/";
const char* TOKEN_DOMAIN_CLAIM = "https://davidbourgault.ca/domain";

api::Result<jwt::jwt_object> GetJWTToken(const grpc::ServerContext& context)
{
    std::string token;

    auto authorization = context.client_metadata().find("authorization");

    if (authorization != context.client_metadata().end()) {
        token = std::string(authorization->second.data(), authorization->second.length());
        if (token.find("Bearer ") != std::string::npos) {
            token = token.substr(7);
        }
    } else {
        return { api::Status::UNAUTHENTICATED, "missing auth token" };
    }

    std::error_code ec;
    jwt::jwt_object object = jwt::decode(
        token,
        jwt::params::algorithms({ "RS256" }),
        ec,
        jwt::params::secret(AUTH0_PUB_KEY),
        jwt::params::issuer(TOKEN_ISSUER),
        jwt::params::aud("ganymede-api"));

    if (ec) {
        return { api::Status::UNAUTHENTICATED, "invalid auth token" };
    } else {
        return { std::move(object) };
    }
}

std::string GetDomain(const jwt::jwt_object& token)
{
    return token.payload().get_claim_value<std::string>(TOKEN_DOMAIN_CLAIM);
}

api::Result<AuthData> JwtAuthValidator::ValidateRequestAuth(const grpc::ServerContext& context) const
{
    auto token = GetJWTToken(context);

    if (token) {
        AuthData data = {
            GetDomain(token.value())
        };
        return { std::move(data) };
    }

    return { token.status(), token.error() };
}

} // namespace ganymede::auth
