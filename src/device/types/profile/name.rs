use crate::{error::Error, name::ResourceName};

pub struct ProfileName(uuid::Uuid);

impl ProfileName {
    pub fn new(id: uuid::Uuid) -> Self {
        ProfileName { 0: id }
    }

    pub fn nil() -> Self {
        ProfileName { 0: uuid::Uuid::nil() }
    }

    pub fn try_from(value: &str) -> Result<Self, Error> {
        Ok(ProfileName {
            0: ResourceName::try_from(value, "profiles/{}")?.get("profiles")?,
        })
    }

    pub fn to_string(self) -> String {
        ResourceName::new(vec![("profiles", self.0)]).to_string()
    }
}

impl TryFrom<&str> for ProfileName {
    type Error = Error;

    fn try_from(value: &str) -> Result<Self, Self::Error> {
        ProfileName::try_from(value)
    }
}

impl From<ProfileName> for String {
    fn from(value: ProfileName) -> Self {
        value.to_string()
    }
}

impl From<ProfileName> for uuid::Uuid {
    fn from(value: ProfileName) -> Self {
        value.0
    }
}
