use sqlx::FromRow;

#[derive(Copy, Clone, Debug, PartialEq, sqlx::Type)]
#[sqlx(type_name = "feature_type", rename_all = "lowercase")]
pub enum FeatureType {
    Light,
}

#[derive(Debug, Clone, PartialEq, FromRow)]
pub struct FeatureModel {
    id: uuid::Uuid,
    display_name: String,
    feature_type: FeatureType,
}

impl FeatureModel {
    pub fn new(id: uuid::Uuid, display_name: String, feature_type: FeatureType) -> Self {
        return FeatureModel {
            id: id,
            display_name: display_name,
            feature_type: feature_type,
        };
    }

    pub fn id(&self) -> uuid::Uuid {
        self.id
    }

    pub fn display_name(&self) -> String {
        self.display_name.clone()
    }

    pub fn feature_type(&self) -> FeatureType {
        self.feature_type
    }
}
