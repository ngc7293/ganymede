#include "jwt.hh"

#include <optional>
#include <string>

#include <grpcpp/grpcpp.h>

#include <jwt/jwt.hpp>

namespace ganymede::common::auth {

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

const char* TOKEN_DOMAIN_CLAIM = "https://davidbourgault.ca/domain";

std::optional<jwt::jwt_object> GetJWTToken(const grpc::ServerContext* context)
{
    std::string token;

    // Google API Gatewat adds it's own authorization JWT, but preservers the
    // original token in this header
    // TODO: This could probably simplified with a ifdef NDEBUG check
    auto authorization = context->client_metadata().find("x-forwarded-authorization");

    // We also check for the simple authorization header (for testing)
    if (authorization == context->client_metadata().end()) {
        authorization = context->client_metadata().find("authorization");
    }

    if (authorization != context->client_metadata().end()) {
        token = std::string(authorization->second.data(), authorization->second.length());
        if (token.find("Bearer ") != std::string::npos) {
            token = token.substr(7);
        }
    }

    std::error_code ec;
    jwt::jwt_object object = jwt::decode(
        token,
        jwt::params::algorithms({"RS256"}),
        ec,
        jwt::params::secret(AUTH0_PUB_KEY),
        jwt::params::issuer("https://dev-ganymede.us.auth0.com/"),
        jwt::params::aud("ganymede-api")
    );

    if (ec) {
        return std::optional<jwt::jwt_object>();
    } else {
        return std::optional<jwt::jwt_object>(object);
    }
}

std::string GetDomain(const jwt::jwt_object& token)
{
    return token.payload().get_claim_value<std::string>(TOKEN_DOMAIN_CLAIM);
}

bool CheckJWTTokenAndGetDomain(const grpc::ServerContext* context, std::string& domain)
{
    auto token = common::auth::GetJWTToken(context);
    if (token.has_value()) {
        domain = common::auth::GetDomain(token.value());
        return true;
    }
    return false;
}

}