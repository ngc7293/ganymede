#[derive(Debug, PartialEq)]
pub enum ConfigError {
    InvalidConfigId,
    InvalidLightConfig,
    NoSuchConfig,
    ConfigInUse,
    DatabaseError(String),
}

impl From<ConfigError> for tonic::Status {
    fn from(value: ConfigError) -> Self {
        match value {
            ConfigError::InvalidConfigId => tonic::Status::invalid_argument("invalid config id"),
            ConfigError::NoSuchConfig => tonic::Status::not_found("no such config"),
            ConfigError::InvalidLightConfig => {
                tonic::Status::invalid_argument("invalid light config")
            }
            ConfigError::ConfigInUse => tonic::Status::failed_precondition("config is in use"),
            ConfigError::DatabaseError(err) => {
                log::error!("{err}");
                tonic::Status::internal("internal error")
            },
        }
    }
}
