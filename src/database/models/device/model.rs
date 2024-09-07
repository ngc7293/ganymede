use sqlx::FromRow;
use uuid::Uuid;

use crate::types::MacAddress;

#[derive(Debug, Clone, PartialEq, FromRow)]
pub struct DeviceModel {
    // Unique identifier
    pub device_id: Uuid,

    // Config identifier for this device
    pub config_id: Uuid,

    // Device's MAC address. Used by devices for self-identification during poll
    #[sqlx(try_from = "String")]
    pub mac: MacAddress,

    // Short display name
    pub display_name: String,

    // Extended user-facing description
    pub description: String,

    // Timezone in tzdata <Region/City> format
    pub timezone: String,
}
