use super::Error;

fn log_and_return_internal_error(message: String) -> tonic::Status {
    log::error!("request failed with database error: {message}");

    if cfg!(debug_assertions) {
        tonic::Status::internal(message.to_string())
    } else {
        tonic::Status::internal("internal error")
    }
}

impl From<Error> for tonic::Status {
    fn from(value: Error) -> Self {
        match value {
            Error::OutOfRange => tonic::Status::invalid_argument("value out of range"),
            Error::BadUuid => tonic::Status::invalid_argument("invalid uuid"),
            Error::BadMacAddress => tonic::Status::invalid_argument("invalid mac address"),
            Error::BadTimezone => tonic::Status::invalid_argument("invalid or unknown timezone"),
            Error::BadTimestamp => tonic::Status::invalid_argument("invalid timestamp"),
            Error::BadPollPeriod => tonic::Status::invalid_argument("invalid poll period"),
            Error::BadLightConfiguration => tonic::Status::invalid_argument("invalid light configuration"),
            Error::BadSensorConfiguration => tonic::Status::invalid_argument("invalid sensor configuration"),
            Error::DuplicateMacAddress => tonic::Status::invalid_argument("duplicate mac address"),
            Error::ConfigInUse => tonic::Status::failed_precondition("config is in use"),
            Error::NoSuchDevice => tonic::Status::not_found("no such device"),
            Error::NoSuchConfig => tonic::Status::not_found("no such config"),
            Error::GenericError(message) => log_and_return_internal_error(message),
            Error::DatabaseError(message) => log_and_return_internal_error(message),
        }
    }
}
