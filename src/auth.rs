use serde::{Deserialize, Serialize};

use jsonwebtoken::{DecodingKey, Validation};
use uuid::Uuid;

const AUTH0_KEY: &str = "
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArV/vHT8ATpXy3Ns/KARs
P+T4In5Ex9k5iAXjkIPlT1+pzm2P5yYLIeGQ+X9jtQ3AGpL7E0xgX96QpIzL7AI5
uSIdcyHOpiGMdgfIknH+Gmg0HCyCsoYflr7L0S0lRSJpEGCKp9WaV1JexLEgZREf
vPz/j5yKR/uAJf+QlhcHgl0/GmxjRgg8Aex3/dSCjsOmiQ62EDTOAfXeu5oec2n+
TFHaVhH3EEbVMXzyAiL6QZKDSvTIqRO2t/M5ogQB7JQko5wVQanlCvAnMHTaUvbQ
4aEu9tyMLqcdx+MKys1Hv1zHw6rA212fTj4hHWnYciA31f9sljjWM5eqAvzro+Q9
BwIDAQAB
-----END PUBLIC KEY-----
";

#[derive(Serialize, Deserialize)]
struct Claims {
    aud: String,
    exp: usize,
    iat: usize,
    iss: String,
    sub: String,

    #[serde(rename(deserialize = "https://davidbourgault.ca/domain"))]
    domain: String,

    #[serde(rename(deserialize = "https://davidbourgault.ca/domain_id"))]
    domain_id: String,
}

pub fn authenticate<T>(request: &tonic::Request<T>) -> Result<Uuid, tonic::Status> {
    let header = match request.metadata().get("authorization") {
        Some(header) => header,
        None => return Err(tonic::Status::unauthenticated("Missing authorization header")),
    };

    let header_ascii = match header.to_str() {
        Ok(ascii) => ascii,
        Err(_) => return Err(tonic::Status::unauthenticated("Invalid authorization token")),
    };

    if !header_ascii.starts_with("Bearer ") {
        return Err(tonic::Status::unauthenticated("Invalid authorization token"));
    }

    let decoding_key = match DecodingKey::from_rsa_pem(AUTH0_KEY.as_bytes()) {
        Ok(key) => key,
        Err(err) => {
            log::error!("Failed to decode Auth0 key: {err}");
            return Err(tonic::Status::internal("Internal error"));
        }
    };

    let token = match jsonwebtoken::decode::<Claims>(
        &header_ascii[7..],
        &decoding_key,
        &Validation::new(jsonwebtoken::Algorithm::RS256),
    ) {
        Ok(token) => token,
        Err(_) => return Err(tonic::Status::unauthenticated("Invalid authorization token")),
    };

    let domain_id = match Uuid::try_parse(&token.claims.domain_id) {
        Ok(header) => header,
        Err(_) => return Err(tonic::Status::unauthenticated("Invalid authorization token")),
    };

    Ok(domain_id)
}
