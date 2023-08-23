use sqlx::FromRow;

#[derive(Debug, Clone, FromRow)]
pub struct ConfigModel {
    pub config_id: uuid::Uuid,
    pub domain_id: uuid::Uuid,
    pub display_name: String,
    pub light_config: serde_json::Value,
}
