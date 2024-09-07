pub mod error;
pub mod tonic;

pub use error::Error;
pub type Result<T, E = Error> = std::result::Result<T, E>;
