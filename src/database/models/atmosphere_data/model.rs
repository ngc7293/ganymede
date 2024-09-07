use chrono::{DateTime, Utc};
use sqlx::FromRow;

use crate::types::{Celsius, RelativeHumidity};

#[derive(Debug, Clone, PartialEq, FromRow)]
pub struct AtmosphereDataModel {
    // Timestamp for data measurement
    pub observed_on: DateTime<Utc>,

    // UUID of the device that produced the measurement
    pub device_id: uuid::Uuid,

    // Air temperature
    pub temperature: Celsius,

    // Air humidity (0-1)
    pub relative_humidity: RelativeHumidity,
}
