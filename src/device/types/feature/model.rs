use sqlx::FromRow;

#[derive(Clone, Debug, PartialEq, sqlx::Type)]
#[sqlx(type_name = "feature_type", rename_all = "lowercase")]
pub enum FeatureType {
    Light,
}

#[derive(Debug, Clone, PartialEq, FromRow)]
pub struct FeatureModel {
    pub id: uuid::Uuid,
    pub display_name: String,
    pub feature_type: FeatureType,
}
