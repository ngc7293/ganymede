use sqlx::FromRow;

use crate::types::mac;

#[derive(Debug, Clone, PartialEq, FromRow)]
pub struct DeviceModel {
    pub device_id: uuid::Uuid,
    pub display_name: String,
    #[sqlx(try_from = "String")]
    pub mac: mac::Mac,
    pub config_id: uuid::Uuid,
    pub description: String,
    pub timezone: String,
}
