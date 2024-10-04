#[derive(Debug, Clone, PartialEq)]
pub enum Error {
    // Value is out of range
    OutOfRange,

    // Errors for various types of parsing errors
    BadUuid,
    BadMacAddress,
    BadTimezone,
    BadTimestamp,
    BadPollPeriod,
    BadLightConfiguration,
    BadSensorConfiguration,

    // There is already a device with this MAC address in the database
    DuplicateMacAddress,

    // Config is referenced by devices and cannot be deleted
    ConfigInUse,

    // Could not find the requested entity
    NoSuchDevice,
    NoSuchConfig,

    // Unhandled database error
    DatabaseError(String),

    // Avoid this, prefer creating a new enum type if necessary
    GenericError(String),
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl std::error::Error for Error {}

// Conversions from commonly found errors
impl From<sqlx::Error> for Error {
    fn from(value: sqlx::Error) -> Self {
        Error::DatabaseError(value.to_string())
    }
}

impl From<uuid::Error> for Error {
    fn from(_: uuid::Error) -> Self {
        Error::BadUuid
    }
}

impl From<chrono_tz::ParseError> for Error {
    fn from(_: chrono_tz::ParseError) -> Self {
        Error::BadTimezone
    }
}
