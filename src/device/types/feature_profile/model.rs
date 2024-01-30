use crate::{error::Error, ganymede};

use crate::device::types::feature::model::FeatureType;

#[derive(Debug, Clone, PartialEq)]
pub enum FeatureProfileConfig {
    Light(ganymede::v2::configurations::LightProfile),
}

impl FeatureProfileConfig {
    pub fn as_json(&self) -> Result<String, Error> {
        let config_parse_err = |_| -> Error {
            Error::DatabaseError(format!("unhandled"))
        };

        let stringified = match self {
            FeatureProfileConfig::Light(c) => serde_json::to_string(c).map_err(config_parse_err)?,
        };

        Ok(stringified)
    }
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
pub struct FeatureProfileModel {
    id: uuid::Uuid,
    display_name: String,
    profile_id: uuid::Uuid,
    feature_id: uuid::Uuid,
    config: FeatureProfileConfig,
}

impl FeatureProfileModel {
    pub fn try_new(
        id: uuid::Uuid,
        display_name: String,
        profile_id: uuid::Uuid,
        feature_id: uuid::Uuid,
        feature_type: FeatureType,
        config: serde_json::Value,
    ) -> Result<Self, Error> {
        let config_parse_err = |_| -> Error {
            Error::DatabaseError(format!("Could not parse config for feature_profile {}", id))
        };

        let parsed_config = match feature_type {
            FeatureType::Light => FeatureProfileConfig::Light(
                serde_json::from_value::<ganymede::v2::configurations::LightProfile>(config)
                    .map_err(config_parse_err)?,
            ),
        };

        Ok(FeatureProfileModel {
            id: id,
            display_name: display_name,
            profile_id: profile_id,
            feature_id: feature_id,
            config: parsed_config,
        })
    }

    pub fn id(&self) -> uuid::Uuid {
        self.id
    }

    pub fn display_name(&self) -> String {
        self.display_name.clone()
    }

    pub fn profile_id(&self) -> uuid::Uuid {
        self.profile_id
    }

    pub fn set_profile_id(&mut self, profile_id: uuid::Uuid) {
        self.profile_id = profile_id
    }

    pub fn feature_id(&self) -> uuid::Uuid {
        self.feature_id
    }

    pub fn config(&self) -> FeatureProfileConfig {
        self.config.clone()
    }
}
