use crate::{Error, Result};

#[derive(Debug, Copy, Clone, PartialEq, PartialOrd, sqlx::Type)]
#[sqlx(transparent)]
pub struct Fractional(f32);

impl TryFrom<f32> for Fractional {
    type Error = Error;

    fn try_from(value: f32) -> Result<Self, Self::Error> {
        if 0.0 <= value && value <= 1.0 {
            Ok(Fractional(value))
        } else {
            Err(Error::OutOfRange)
        }
    }
}

impl Into<f32> for Fractional {
    fn into(self) -> f32 {
        self.0
    }
}
