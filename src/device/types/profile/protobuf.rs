use crate::{ganymede::{self}, device::types::feature_profile::model::FeatureProfileModel};

use super::{model::ProfileModel, name::ProfileName};
use crate::error::Error;

impl TryFrom<ganymede::v2::Profile> for ProfileModel {
    type Error = Error;

    fn try_from(value: ganymede::v2::Profile) -> Result<Self, Self::Error> {
        let feature = Self::new(
            ProfileName::try_from(&value.name)?.into(),
            uuid::Uuid::nil(),
            value.display_name,
            value.feature_profiles.into_iter().map(
                |fp| FeatureProfileModel::try_from(fp)
            ).collect::<Result<Vec<FeatureProfileModel>, Error>>()?,
        );

        Ok(feature)
    }
}

impl From<ProfileModel> for ganymede::v2::Profile {
    fn from(value: ProfileModel) -> Self {
        return Self {
            name: ProfileName::new(value.id()).into(),
            display_name: value.display_name(),
            feature_profiles: value.feature_profiles().into_iter().map(|fp| fp.into()).collect(),
        };
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_parse_profile() {
        let profile = ganymede::v2::Profile {
            name: "profiles/lHQ-oH4nRuuHNXlaG2NYGA".into(),
            display_name: "A profile".into(),
            feature_profiles: Vec::new(),
        };

        let expected = ProfileModel::new(uuid::uuid!("94743ea0-7e27-46eb-8735-795a1b635818"), uuid::Uuid::nil(), "A profile".into(), Vec::new());

        assert_eq!(ProfileModel::try_from(profile).unwrap(), expected);
    }

    #[test]
    fn test_parse_feature_with_bad_name() {
        let profile = ganymede::v2::Profile {
            name: "device/lHQ-oH4nRuuHNXlaG2NYGA".into(),
            display_name: "A feature".into(),
            feature_profiles: Vec::new(),
        };

        assert_eq!(ProfileModel::try_from(profile).unwrap_err(), Error::NameError);
    }

    #[test]
    fn test_serialize_model() {
        let feature = ProfileModel::new(
            uuid::uuid!("94743ea0-7e27-46eb-8735-795a1b635818"),
            uuid::Uuid::nil(),
            "some string\nyeah".into(),
            Vec::new(),
        );

        assert_eq!(
            ganymede::v2::Profile::from(feature),
            ganymede::v2::Profile {
                name: "profiles/lHQ-oH4nRuuHNXlaG2NYGA".into(),
                display_name: "some string\nyeah".into(),
                feature_profiles: Vec::new()
            }
        )
    }
}
