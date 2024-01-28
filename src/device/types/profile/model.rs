use sqlx::FromRow;

use crate::ganymede::v2::FeatureProfile;

#[derive(Clone, Debug, PartialEq, sqlx::Type)]
#[sqlx(type_name = "feature_type", rename_all = "lowercase")]
pub enum FeatureType {
    Light,
}

#[derive(Debug, Clone, PartialEq, FromRow)]
pub struct ProfileModel {
    id: uuid::Uuid,
    display_name: String,
}

impl ProfileModel {
    pub fn new(id: uuid::Uuid, display_name: String, feature_profiles: Vec<FeatureProfile>) -> Self {
        return ProfileModel {
            id: id,
            display_name: display_name
        };
    }

    pub fn id(&self) -> uuid::Uuid {
        self.id
    }

    pub fn display_name(&self) -> String {
        self.display_name.clone()
    }
}
