#[derive(Debug, PartialEq)]
pub enum DeviceError {
    InvalidMac,
    InvalidTimezone,
    InvalidDeviceId,
    InvalidConfigId,
    NoSuchDevice,
    NoSuchConfig,
    MacConflict,
    DatabaseError(String),
}

impl From<DeviceError> for tonic::Status {
    fn from(value: DeviceError) -> Self {
        match value {
            DeviceError::InvalidConfigId => tonic::Status::invalid_argument("invalid config id"),
            DeviceError::InvalidDeviceId => tonic::Status::invalid_argument("invalid device id"),
            DeviceError::InvalidTimezone => tonic::Status::invalid_argument("invalid timezone"),
            DeviceError::InvalidMac => tonic::Status::invalid_argument("invalid mac address"),
            DeviceError::NoSuchDevice => tonic::Status::not_found("no such device"),
            DeviceError::NoSuchConfig => tonic::Status::not_found("no such config"),
            DeviceError::MacConflict => {
                tonic::Status::invalid_argument("mac address already in use")
            }
            DeviceError::DatabaseError(_) => tonic::Status::internal("internal error"),
        }
    }
}
