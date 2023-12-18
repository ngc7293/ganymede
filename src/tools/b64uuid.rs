use base64::{prelude::BASE64_URL_SAFE_NO_PAD as base64, Engine};
use std::env;

enum Error {
    GenericError,
}

fn try_b64_to_uuid(v: &str) -> Result<uuid::Uuid, Error> {
    if let Ok(decoded) = base64.decode(v) {
        if let Ok(uuid) = uuid::Uuid::try_from(decoded) {
            Ok(uuid)
        } else {
            Err(Error::GenericError)
        }
    } else {
        Err(Error::GenericError)
    }
}

fn try_uuid_to_b64(v: &str) -> Result<String, Error> {
    if let Ok(uuid) = uuid::Uuid::try_from(v) {
        Ok(base64.encode(uuid))
    } else {
        Err(Error::GenericError)
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();

    for arg in args.into_iter().skip(1) {
        if let Ok(uuid) = try_b64_to_uuid(&arg) {
            println!("{}", uuid.to_string())
        } else if let Ok(b64) = try_uuid_to_b64(&arg) {
            println!("{}", b64)
        } else {
            println!("Bad input")
        }
    }
}
