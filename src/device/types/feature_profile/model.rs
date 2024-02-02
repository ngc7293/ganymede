use async_trait::async_trait;
use sqlx::PgConnection;
use uuid::Uuid;

use crate::device::types::feature::name::FeatureName;
use crate::device::types::model::DomainDatabaseModel;
use crate::device::types::protobuf::ToProtobuf;
use crate::{error::Error, ganymede};

use crate::device::types::feature::model::FeatureType;

use super::name::FeatureProfileName;

#[derive(Debug, Clone, PartialEq)]
pub enum FeatureProfileConfig {
    Light(ganymede::v2::configurations::LightProfile),
}

impl FeatureProfileConfig {
    pub fn as_json(&self) -> Result<String, Error> {
        let config_parse_err = |_| -> Error { Error::DatabaseError(format!("unhandled")) };

        let stringified = match self {
            FeatureProfileConfig::Light(c) => serde_json::to_string(c).map_err(config_parse_err)?,
        };

        Ok(stringified)
    }
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
pub struct FeatureProfileModel {
    id: Uuid,
    domain_id: Uuid,
    display_name: String,
    profile_id: Uuid,
    feature_id: Uuid,
    config: FeatureProfileConfig,
}

impl FeatureProfileModel {
    pub fn try_new(
        id: Uuid,
        domain_id: Uuid,
        display_name: String,
        profile_id: Uuid,
        feature_id: Uuid,
        feature_type: FeatureType,
        config: serde_json::Value,
    ) -> Result<Self, Error> {
        let config_parse_err =
            |_| -> Error { Error::DatabaseError(format!("Could not parse config for feature_profile {}", id)) };

        let parsed_config = match feature_type {
            FeatureType::Light => FeatureProfileConfig::Light(
                serde_json::from_value::<ganymede::v2::configurations::LightProfile>(config)
                    .map_err(config_parse_err)?,
            ),
        };

        Ok(FeatureProfileModel {
            id: id,
            domain_id,
            display_name: display_name,
            profile_id: profile_id,
            feature_id: feature_id,
            config: parsed_config,
        })
    }

    pub fn id(&self) -> Uuid {
        self.id
    }

    pub fn display_name(&self) -> String {
        self.display_name.clone()
    }

    pub fn profile_id(&self) -> Uuid {
        self.profile_id
    }

    pub fn set_profile_id(&mut self, profile_id: Uuid) {
        self.profile_id = profile_id
    }

    pub fn feature_id(&self) -> Uuid {
        self.feature_id
    }

    pub fn config(&self) -> FeatureProfileConfig {
        self.config.clone()
    }

    pub async fn fetch_all_with_profile(
        conn: &mut PgConnection,
        profile_id: Uuid,
        domain_id: Uuid,
    ) -> Result<Vec<FeatureProfileModel>, Error>
    {
        let rows = sqlx::query_as::<_, (Uuid, String, Uuid, Uuid, FeatureType, String)>(
            "
            SELECT feature_profiles.id, feature_profiles.display_name, feature_profiles.profile_id, feature_profiles.feature_id, features.feature_type, feature_profiles.config::text
            FROM feature_profiles
            INNER JOIN features ON features.id = feature_profiles.feature_id
            WHERE
                features.domain_id = $1
                AND feature_profiles.domain_id = $1
                AND feature.profiles.profile_id = $2
        "
    )
        .bind(&domain_id)
        .bind(&profile_id)
        .fetch_all(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        let feature_profiles = rows.into_iter().map(
            |(id, display_name, profile_id, feature_id, feature_type, config)| -> Result<FeatureProfileModel, Error> {
                FeatureProfileModel::try_new(id, domain_id, display_name, profile_id, feature_id, feature_type, serde_json::from_str(config.as_str()).unwrap())
            }
        ).collect::<Result<Vec<FeatureProfileModel>, Error>>()?;
        Ok(feature_profiles)
    }
}

#[async_trait]
impl DomainDatabaseModel for FeatureProfileModel {
    type PkType = Uuid;

    async fn create(&mut self, conn: &mut PgConnection) -> Result<(), Error>

    {
        let (feature_id,) = sqlx::query_as::<_, (Uuid,)>(
            "
            INSERT INTO feature_profiles (domain_id, display_name, profile_id, feature_id, config)
            VALUES ($1, $2, $3, $4, $5::JSONB)
            RETURNING id
            ",
        )
        .bind(&self.domain_id)
        .bind(&self.display_name)
        .bind(&self.profile_id)
        .bind(&self.feature_id)
        .bind(&self.config.as_json()?)
        .fetch_one(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        self.id = feature_id;
        Ok(())
    }

    async fn save(&mut self, conn: &mut PgConnection) -> Result<(), Error>

    {
        let _result = sqlx::query(
            "
            UPDATE profiles
            SET
                display_name = $1,
                profile_id = $2,
                feature_id = $3,
                config = $4
            WHERE
                domain_id = $5
                AND profile_id = $6
                AND id = $7
            ",
        )
        .bind(&self.display_name)
        .bind(&self.profile_id)
        .bind(&self.feature_id)
        .bind(&self.config.as_json()?)
        .bind(&self.domain_id)
        .bind(&self.profile_id)
        .bind(&self.id)
        .execute(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        Ok(())
    }

    async fn fetch_one(conn: &mut PgConnection, pk: Self::PkType, domain_id: Uuid) -> Result<Self, Error>

    {
        let feature_profile = match sqlx::query_as::<_, (Uuid, String, Uuid, Uuid, FeatureType, String)>(
            "
            SELECT
                feature_profiles.id, feature_profiles.display_name, profile_id, feature_id, feature_type, config::text
            FROM feature_profiles
            INNER JOIN features ON features.id = feature_profiles.feature_id
            WHERE
                features.domain_id = $1
                AND feature_profiles.domain_id = $1
                AND feature_profiles.profile_id = $2
                AND feature_profiles.id = $3",
        )
        .bind(&domain_id)
        .bind(&pk)
        .fetch_one(&mut *conn)
        .await
        {
            Ok((id, display_name, profile_id, feature_id, feature_type, config)) => FeatureProfileModel::try_new(
                id,
                domain_id,
                display_name,
                profile_id,
                feature_id,
                feature_type,
                serde_json::from_str(config.as_str()).unwrap(),
            )?,
            Err(err) => match err {
                sqlx::Error::RowNotFound => return Err(Error::NoSuchResourceError("FeatureProfile", pk.clone())),
                _ => return Err(Error::DatabaseError(err.to_string())),
            },
        };

        Ok(feature_profile)
    }

    async fn fetch_all(conn: &mut PgConnection, domain_id: Uuid) -> Result<Vec<Self>, Error>

    {
        let result = sqlx::query_as::<_, (Uuid, String, Uuid, Uuid, FeatureType, String)>(
            "
            SELECT feature_profiles.id, feature_profiles.display_name, feature_profiles.profile_id, feature_profiles.feature_id, features.feature_type, feature_profiles.config::text
            FROM feature_profiles\
            INNER JOIN features ON features.id = feature_profiles.feature_id
            WHERE
                features.domain_id = $1
                AND feature_profiles.domain_id = $1
            "
        )
        .bind(&domain_id)
        .fetch_all(&mut *conn)
        .await;

        let feature_profiles = match result {
            Ok(rows) => rows.into_iter().map(|(id, display_name, profile_id, feature_id, feature_type, config)| -> Result<FeatureProfileModel, Error> {
                FeatureProfileModel::try_new(id, domain_id, display_name, profile_id, feature_id, feature_type, serde_json::from_str(config.as_str()).unwrap())
            }).collect::<Result<Vec<FeatureProfileModel>, Error>>()?,
            Err(err) => match err {
                _ => return Err(Error::DatabaseError(err.to_string())),
            },
        };

        Ok(feature_profiles)
    }

    async fn delete(conn: &mut PgConnection, pk: Self::PkType, domain_id: Uuid) -> Result<(), Error>

    {
        let _result = sqlx::query("DELETE FROM feature_profiles WHERE id = $1 AND domain_id = $2")
            .bind(pk)
            .bind(domain_id)
            .execute(&mut *conn)
            .await
            .map_err(|err| Error::DatabaseError(err.to_string()))?;

        Ok(())
    }
}

impl ToProtobuf for FeatureProfileModel {
    type Output = ganymede::v2::FeatureProfile;

    fn to_protobuf(self, _include_nested: bool) -> Self::Output {
        let config = match self.config() {
            FeatureProfileConfig::Light(config) => ganymede::v2::feature_profile::Config::LightProfile(config),
        };

        Self::Output {
            name: FeatureProfileName::new(self.profile_id(), self.id()).into(),
            display_name: self.display_name(),
            feature_name: FeatureName::new(self.feature_id()).into(),
            feature: None,
            config: Some(config),
        }
    }
}

impl ToProtobuf for Vec<FeatureProfileModel> {
    type Output = Vec<ganymede::v2::FeatureProfile>;

    fn to_protobuf(self, include_nested: bool) -> Self::Output {
        self.into_iter().map(|x| x.to_protobuf(include_nested)).collect()
    }
}
