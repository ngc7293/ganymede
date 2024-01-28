use crate::{ganymede, device::types::feature::{model::FeatureType, name::FeatureName}};

use super::{model::{FeatureProfileModel, FeatureProfileConfig}, name::FeatureProfileName};
use crate::error::Error;

impl TryFrom<ganymede::v2::FeatureProfile> for FeatureProfileModel {
    type Error = Error;

    fn try_from(value: ganymede::v2::FeatureProfile) -> Result<Self, Self::Error> {
        let (feature_type, config) = match value.config {
            Some(config) => match config {
                ganymede::v2::feature_profile::Config::LightProfile(light_profile) => (FeatureType::Light, light_profile),
            },
            None => Err(Error::ValueError("None".into(), "FeatureProfile.config"))?,
        };

        let (profile_id, feature_profile_id) = FeatureProfileName::try_from(&value.name)?.into();
        let feature_id = FeatureName::try_from(&value.feature_name)?.into();

        let feature_profile = FeatureProfileModel::try_new(
            feature_profile_id,
            value.display_name,
            profile_id,
            feature_id,
            feature_type,
            serde_json::to_value(config).map_err(|_| Error::DatabaseError("internal".into()))?,
        )?;

        Ok(feature_profile)
    }
}

impl From<FeatureProfileModel> for ganymede::v2::FeatureProfile {
    fn from(value: FeatureProfileModel) -> Self {
        let config = match value.config() {
            FeatureProfileConfig::Light(config) => ganymede::v2::feature_profile::Config::LightProfile(config),
        };

        Self {
            name: FeatureProfileName::new(value.profile_id(), value.id()).into(),
            display_name: value.display_name(),
            feature_name: FeatureName::new(value.feature_id()).into(),
            feature: None,
            config: Some(config),
        }
    }
}

// #[cfg(test)]
// mod test {
//     use super::*;

//     #[test]
//     fn test_parse_profile() {
//         let profile = ganymede::v2::Profile {
//             name: "profiles/lHQ-oH4nRuuHNXlaG2NYGA".into(),
//             display_name: "A profile".into(),
//             feature_profiles: Vec::new(),
//         };

//         let expected = ProfileModel::new(uuid::uuid!("94743ea0-7e27-46eb-8735-795a1b635818"), "A profile".into());

//         assert_eq!(ProfileModel::try_from(profile).unwrap(), expected);
//     }

//     #[test]
//     fn test_parse_feature_with_bad_name() {
//         let profile = ganymede::v2::Profile {
//             name: "device/lHQ-oH4nRuuHNXlaG2NYGA".into(),
//             display_name: "A feature".into(),
//             feature_profiles: Vec::new(),
//         };

//         assert_eq!(ProfileModel::try_from(profile).unwrap_err(), Error::NameError);
//     }

//     #[test]
//     fn test_serialize_model() {
//         let feature = ProfileModel::new(
//             uuid::uuid!("94743ea0-7e27-46eb-8735-795a1b635818"),
//             "some string\nyeah".into(),
//         );

//         assert_eq!(
//             ganymede::v2::Profile::from(feature),
//             ganymede::v2::Profile {
//                 name: "profiles/lHQ-oH4nRuuHNXlaG2NYGA".into(),
//                 display_name: "some string\nyeah".into(),
//                 feature_profiles: Vec::new()
//             }
//         )
//     }
// }
