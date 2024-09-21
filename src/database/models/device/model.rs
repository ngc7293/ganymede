use chrono::{DateTime, TimeDelta, Utc};
use sqlx::postgres::PgRow;
use sqlx::Row;
use uuid::Uuid;

use crate::types::MacAddress;

#[derive(Debug, Clone, PartialEq)]
pub struct DeviceModel {
    // Unique identifier
    pub device_id: Uuid,

    // Config identifier for this device
    pub config_id: Uuid,

    // Device's MAC address. Used by devices for self-identification during poll
    pub mac: MacAddress,

    // Short display name
    pub display_name: String,

    // Extended user-facing description
    pub description: String,

    // Timezone in tzdata <Region/City> format
    pub timezone: String,

    // Output only. Timestamp of last PollRequest received by the server
    // This can only be set by calling update_device_last_poll; it is ignored in inserts and updates.
    pub last_poll: Option<DateTime<Utc>>,

    // Output only. Time since last restart.
    // This can only be set by calling update_device_uptime; it is ignored in inserts and updates.
    pub uptime: Option<TimeDelta>,
}

impl<'r> sqlx::FromRow<'r, PgRow> for DeviceModel {
    fn from_row(row: &'r PgRow) -> Result<Self, sqlx::Error> {
        let device_id = row.try_get("device_id")?;
        let config_id = row.try_get("config_id")?;
        let display_name = row.try_get("display_name")?;
        let description = row.try_get("description")?;
        let timezone = row.try_get("timezone")?;
        let last_poll = row.try_get("last_poll")?;

        let mac: String = row.try_get("mac")?;
        let mac: MacAddress = mac.try_into().map_err(|err| sqlx::Error::Decode(Box::new(err)))?;

        let uptime: Option<sqlx::postgres::types::PgInterval> = row.try_get("uptime")?;
        let uptime: Option<TimeDelta> = match uptime {
            Some(interval) => Some(chrono::TimeDelta::nanoseconds(interval.microseconds * 1000)),
            None => None,
        };

        Ok(DeviceModel {
            device_id,
            config_id,
            mac,
            display_name,
            description,
            timezone,
            last_poll,
            uptime,
        })
    }
}
