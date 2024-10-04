use chrono::TimeDelta;
use sqlx::postgres::PgRow;
use sqlx::Row;
use uuid::Uuid;

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigModel {
    // Unique identifier
    pub config_id: Uuid,

    // Short display name
    pub display_name: String,

    // How long the device should wait between polls
    pub poll_period: TimeDelta,

    // Configuration for lights
    pub light_config: serde_json::Value,

    // Configuration for sensors
    pub sensor_configs: serde_json::Value,
}

impl<'r> sqlx::FromRow<'r, PgRow> for ConfigModel {
    fn from_row(row: &'r PgRow) -> Result<Self, sqlx::Error> {
        let config_id = row.try_get("config_id")?;
        let display_name = row.try_get("display_name")?;
        let light_config = row.try_get("light_config")?;
        let sensor_configs = row.try_get("sensor_configs")?;

        let poll_period_pg: sqlx::postgres::types::PgInterval = row.try_get("poll_period")?;
        let poll_period = chrono::TimeDelta::nanoseconds(poll_period_pg.microseconds * 1000);

        Ok(ConfigModel {
            config_id,
            display_name,
            poll_period,
            light_config,
            sensor_configs,
        })
    }
}
