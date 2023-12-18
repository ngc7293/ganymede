#[derive(Debug, PartialEq)]
pub enum Error {
    NameError,

    // Invalid value <> for type <>
    ValueError(String, &'static str),

    // No Resource of type <String> with id <Uuid>
    NoSuchResourceError(&'static str, uuid::Uuid),

    // For unhandled database errors
    DatabaseError(String),
}

impl From<Error> for tonic::Status {
    fn from(value: Error) -> Self {
        match value {
            Error::NameError => tonic::Status::invalid_argument("invalid name"),
            Error::ValueError(value, r#type) => {
                tonic::Status::invalid_argument(format!("invalid value '{}' for type {}", value, r#type))
            }
            Error::NoSuchResourceError(r#type, id) => {
                tonic::Status::not_found(format!("no such resource {}/{}", r#type, id))
            }
            Error::DatabaseError(err) => {
                log::error!("{err}");
                tonic::Status::internal("internal error")
            }
        }
    }
}
