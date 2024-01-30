use crate::device::types::feature_profile::model::FeatureProfileModel;

#[derive(Clone, Debug, PartialEq, sqlx::Type)]
#[sqlx(type_name = "feature_type", rename_all = "lowercase")]
pub enum FeatureType {
    Light,
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
pub struct ProfileModel {
    id: uuid::Uuid,
    display_name: String,

    #[sqlx(skip)]
    feature_profiles: Vec<FeatureProfileModel>,
}

impl ProfileModel {
    pub fn new(id: uuid::Uuid, display_name: String, feature_profiles: Vec<FeatureProfileModel>) -> Self {
        return ProfileModel {
            id: id,
            display_name: display_name,
            feature_profiles: feature_profiles,
        };
    }

    pub fn id(&self) -> uuid::Uuid {
        self.id
    }

    pub fn display_name(&self) -> String {
        self.display_name.clone()
    }

    pub fn feature_profiles(&self) -> Vec<FeatureProfileModel> {
        self.feature_profiles.clone()
    }

    pub fn set_feature_profiles(&mut self, feature_profiles: Vec<FeatureProfileModel>) {
        self.feature_profiles = feature_profiles
    }
}
