#[derive(Debug, PartialEq)]
pub enum DeviceError {
    InvalidMac,
    InvalidTimezone,
    InvalidDeviceId,
    InvalidDeviceTypeId,
    NoSuchDevice,
    NoSuchDeviceType,
    MacConflict,
    DatabaseError(String),
}

impl From<DeviceError> for tonic::Status {
    fn from(value: DeviceError) -> Self {
        match value {
            DeviceError::InvalidDeviceTypeId => tonic::Status::invalid_argument("invalid device type id"),
            DeviceError::InvalidDeviceId => tonic::Status::invalid_argument("invalid device id"),
            DeviceError::InvalidTimezone => tonic::Status::invalid_argument("invalid timezone"),
            DeviceError::InvalidMac => tonic::Status::invalid_argument("invalid mac address"),
            DeviceError::NoSuchDevice => tonic::Status::not_found("no such device"),
            DeviceError::NoSuchDeviceType => tonic::Status::not_found("no such device type"),
            DeviceError::MacConflict => tonic::Status::invalid_argument("mac address already in use"),
            DeviceError::DatabaseError(err) => {
                log::error!("{err}");
                tonic::Status::internal("internal error")
            }
        }
    }
}
