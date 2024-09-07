use crate::{Error, Result};

#[derive(Debug, Copy, Clone, PartialEq, PartialOrd, sqlx::Type)]
#[sqlx(transparent)]
pub struct Celsius(f32);

impl TryFrom<f32> for Celsius {
    type Error = Error;

    fn try_from(value: f32) -> Result<Self, Self::Error> {
        if value < -273.15 {
            Err(Error::OutOfRange)
        } else {
            Ok(Celsius(value))
        }
    }
}

impl Into<f32> for Celsius {
    fn into(self) -> f32 {
        self.0
    }
}
