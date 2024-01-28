use crate::{error::Error, name::ResourceName};

pub struct FeatureName(uuid::Uuid);

impl FeatureName {
    pub fn new(id: uuid::Uuid) -> Self {
        FeatureName { 0: id }
    }

    pub fn nil() -> Self {
        FeatureName { 0: uuid::Uuid::nil() }
    }

    pub fn try_from(value: &str) -> Result<Self, Error> {
        Ok(FeatureName {
            0: ResourceName::try_from(value, "features/{}")?.get("features")?,
        })
    }

    pub fn to_string(self) -> String {
        ResourceName::new(vec![("features", self.0)]).to_string()
    }
}

impl TryFrom<&str> for FeatureName {
    type Error = Error;

    fn try_from(value: &str) -> Result<Self, Self::Error> {
        FeatureName::try_from(value)
    }
}

impl From<FeatureName> for String {
    fn from(value: FeatureName) -> Self {
        value.to_string()
    }
}

impl From<FeatureName> for uuid::Uuid {
    fn from(value: FeatureName) -> Self {
        value.0
    }
}
