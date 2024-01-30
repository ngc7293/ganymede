use crate::{error::Error, name::ResourceName};

pub struct FeatureProfileName(uuid::Uuid, uuid::Uuid);

impl FeatureProfileName {
    pub fn new(profile_id: uuid::Uuid, feature_profile_id: uuid::Uuid) -> Self {
        FeatureProfileName { 0: profile_id, 1: feature_profile_id}
    }

    pub fn nil(profile_id: uuid::Uuid) -> Self {
        FeatureProfileName { 0: profile_id, 1: uuid::Uuid::nil() }
    }

    pub fn try_from(value: &str) -> Result<Self, Error> {
        let parsed = ResourceName::try_from(value, "profiles/{}/features/{}")?;

        Ok(FeatureProfileName {
            0: parsed.get("profiles")?,
            1: parsed.get("features")?,
        })
    }

    pub fn to_string(self) -> String {
        ResourceName::new(vec![("profiles", self.0), ("features", self.1)]).to_string()
    }
}

impl TryFrom<&str> for FeatureProfileName {
    type Error = Error;

    fn try_from(value: &str) -> Result<Self, Self::Error> {
        FeatureProfileName::try_from(value)
    }
}

impl From<FeatureProfileName> for String {
    fn from(value: FeatureProfileName) -> Self {
        value.to_string()
    }
}

impl From<FeatureProfileName> for (uuid::Uuid, uuid::Uuid) {
    fn from(value: FeatureProfileName) -> Self {
        (value.0, value.1)
    }
}
