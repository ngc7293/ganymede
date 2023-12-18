use crate::ganymede;

use super::{
    model::{FeatureModel, FeatureType},
    name::FeatureName,
};
use crate::error::Error;

impl TryFrom<i32> for FeatureType {
    type Error = crate::error::Error;

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match ganymede::v2::FeatureType::from_i32(value) {
            Some(protobuf_enum) => protobuf_enum.try_into(),
            None => Err(Error::ValueError(value.to_string(), "FeatureType")),
        }
    }
}

impl TryFrom<ganymede::v2::FeatureType> for FeatureType {
    type Error = crate::error::Error;

    fn try_from(value: ganymede::v2::FeatureType) -> Result<Self, Self::Error> {
        match value {
            ganymede::v2::FeatureType::Unspecified => Err(Error::ValueError("Unspecified".into(), "FeatureType")),
            ganymede::v2::FeatureType::Light => Ok(FeatureType::Light),
        }
    }
}

impl From<FeatureType> for ganymede::v2::FeatureType {
    fn from(value: FeatureType) -> Self {
        match value {
            FeatureType::Light => ganymede::v2::FeatureType::Light,
        }
    }
}

impl TryFrom<ganymede::v2::Feature> for FeatureModel {
    type Error = crate::error::Error;

    fn try_from(value: ganymede::v2::Feature) -> Result<Self, Self::Error> {
        let feature = Self {
            id: FeatureName::try_from(&value.name)?.into(),
            display_name: value.display_name,
            feature_type: value.feature_type.try_into()?,
        };

        Ok(feature)
    }
}

impl From<FeatureModel> for ganymede::v2::Feature {
    fn from(value: FeatureModel) -> Self {
        return Self {
            name: FeatureName::new(value.id).into(),
            display_name: value.display_name,
            feature_type: ganymede::v2::FeatureType::from(value.feature_type).into(),
        };
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_parse_feature() {
        let feature = ganymede::v2::Feature {
            name: "features/lHQ-oH4nRuuHNXlaG2NYGA".into(),
            display_name: "A feature".into(),
            feature_type: 1,
        };

        let expected = FeatureModel {
            id: uuid::uuid!("94743ea0-7e27-46eb-8735-795a1b635818"),
            display_name: "A feature".into(),
            feature_type: FeatureType::Light,
        };

        assert_eq!(FeatureModel::try_from(feature).unwrap(), expected);
    }

    #[test]
    fn test_parse_feature_with_bad_name() {
        let feature = ganymede::v2::Feature {
            name: "device/lHQ-oH4nRuuHNXlaG2NYGA".into(),
            display_name: "A feature".into(),
            feature_type: 1,
        };

        assert_eq!(FeatureModel::try_from(feature).unwrap_err(), Error::NameError);
    }

    #[test]
    fn test_parse_feature_with_unspecified_type() {
        let feature = ganymede::v2::Feature {
            name: "features/lHQ-oH4nRuuHNXlaG2NYGA".into(),
            display_name: "A feature".into(),
            feature_type: 0,
        };

        assert_eq!(
            FeatureModel::try_from(feature).unwrap_err(),
            Error::ValueError("Unspecified".into(), "FeatureType")
        );
    }

    #[test]
    fn test_parse_feature_with_invalid_type() {
        let feature = ganymede::v2::Feature {
            name: "features/lHQ-oH4nRuuHNXlaG2NYGA".into(),
            display_name: "A feature".into(),
            feature_type: 999,
        };

        assert_eq!(
            FeatureModel::try_from(feature).unwrap_err(),
            Error::ValueError("999".into(), "FeatureType")
        );
    }

    #[test]
    fn test_serialize_model() {
        let feature = FeatureModel {
            id: uuid::uuid!("94743ea0-7e27-46eb-8735-795a1b635818"),
            display_name: "some string\nyeah".into(),
            feature_type: FeatureType::Light,
        };

        assert_eq!(
            ganymede::v2::Feature::from(feature),
            ganymede::v2::Feature {
                name: "features/lHQ-oH4nRuuHNXlaG2NYGA".into(),
                display_name: "some string\nyeah".into(),
                feature_type: ganymede::v2::FeatureType::Light.into()
            }
        )
    }
}
