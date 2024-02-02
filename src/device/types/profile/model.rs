use std::collections::HashMap;

use async_trait::async_trait;
use sqlx::PgConnection;
use uuid::Uuid;

use crate::{device::types::{feature::model::FeatureType, feature_profile::model::FeatureProfileModel, model::DomainDatabaseModel, protobuf::{ToProtobuf, TryFromProtobuf}}, error::Error, ganymede};

use super::name::ProfileName;

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
pub struct ProfileModel {
    id: Uuid,
    domain_id: Uuid,
    display_name: String,

    #[sqlx(skip)]
    feature_profiles: Vec<FeatureProfileModel>,
}

impl ProfileModel {
    pub fn new(id: Uuid, domain_id: Uuid, display_name: String, feature_profiles: Vec<FeatureProfileModel>) -> Self {
        return ProfileModel {
            id: id,
            domain_id: domain_id,
            display_name: display_name,
            feature_profiles: feature_profiles,
        };
    }

    pub fn id(&self) -> Uuid {
        self.id
    }

    pub fn display_name(&self) -> String {
        self.display_name.clone()
    }

    pub fn feature_profiles(&self) -> Vec<FeatureProfileModel> {
        self.feature_profiles.clone()
    }

    pub fn add_feature_profile(&mut self, feature_profile: FeatureProfileModel) {
        self.feature_profiles.push(feature_profile)
    }

    pub fn set_feature_profiles(&mut self, feature_profiles: Vec<FeatureProfileModel>) {
        self.feature_profiles = feature_profiles
    }
}

#[async_trait]
impl DomainDatabaseModel for ProfileModel {
    type PkType = Uuid;

    async fn create(&mut self, conn: &mut PgConnection) -> Result<(), Error>
    {
        let (feature_id,) = sqlx::query_as::<_, (Uuid,)>(
            "INSERT INTO profiles (domain_id, display_name) VALUES ($1, $2) RETURNING id",
        )
        .bind(&self.domain_id)
        .bind(&self.display_name)
        .fetch_one(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        self.id = feature_id;
        Ok(())
    }

    async fn save(&mut self, conn: &mut PgConnection) -> Result<(), Error>

    {
        let _result = sqlx::query(
            "UPDATE features SET display_name = $1 WHERE id = $3 AND domain_id = $4",
        )
        .bind(&self.display_name)
        .bind(&self.domain_id)
        .bind(&self.id)
        .fetch_one(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        Ok(())
    }

    async fn fetch_one(conn: &mut PgConnection, pk: Self::PkType, domain_id: Uuid) -> Result<Self, Error>

    {
        let mut profile = sqlx::query_as::<_, ProfileModel>(
            "SELECT id, domain_id, display_name FROM features WHERE id = $1 AND domain_id = $2"
        )
        .bind(pk)
        .bind(domain_id)
        .fetch_one(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        let feature_profiles = FeatureProfileModel::fetch_all_with_profile(&mut *conn, pk, domain_id).await?;
        profile.set_feature_profiles(feature_profiles);

        Ok(profile)
    }

    async fn fetch_all(conn: &mut PgConnection, domain_id: Uuid) -> Result<Vec<Self>, Error>

    {
        let profiles = sqlx::query_as::<_, ProfileModel>(
            "SELECT id, domain_id, display_name FROM profiles WHERE domain_id = $1"
        )
        .bind(domain_id)
        .fetch_all(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        let mut profiles_mapped: HashMap<Uuid, ProfileModel> = profiles.into_iter().map(|p| (p.id(), p)).collect();

        let feature_profiles = sqlx::query_as::<_, (Uuid, String, Uuid, Uuid, FeatureType, String)>(
            "
            SELECT feature_profiles.id, feature_profiles.display_name, feature_profiles.profile_id, feature_profiles.feature_id, features.feature_type, feature_profiles.config::text
            FROM feature_profiles
            INNER JOIN features ON features.id = feature_profiles.feature_id
            WHERE
                features.domain_id = $1
                AND feature_profiles.domain_id = $1
                AND feature_profiles.profile_id = ANY($2)
            "
        )
        .bind(domain_id)
        .bind(profiles_mapped.keys().cloned().collect::<Vec<Uuid>>())
        .fetch_all(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        for feature_profile in feature_profiles.into_iter() {
            let (id, display_name, profile_id, feature_id, feature_type, config) = feature_profile;

            let model = FeatureProfileModel::try_new(id, domain_id, display_name, profile_id, feature_id, feature_type, serde_json::from_str(config.as_str()).unwrap())?;
            if let Some(profile) = profiles_mapped.get_mut(&profile_id) {
                profile.add_feature_profile(model);
            }
        }

        Ok(profiles_mapped.into_iter().map(|(_, v)| v).collect())
    }

    async fn delete(conn: &mut PgConnection, pk: Self::PkType, domain_id: Uuid) -> Result<(), Error>

    {
        let _result = sqlx::query(
            "DELETE FROM features WHERE id = $1 AND domain_id = $2"
        )
        .bind(pk)
        .bind(domain_id)
        .execute(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        Ok(())
    }
}


impl TryFromProtobuf for ProfileModel {
    type Input = ganymede::v2::Profile;

    fn try_from_protobuf(input: Self::Input, domain_id: Uuid, strip_names: bool) -> Result<Self, Error> {
        let id = match strip_names {
            true => Uuid::nil(),
            false => ProfileName::try_from(&input.name)?.into(),
        };

        let feature = ProfileModel::new(
            id,
            domain_id,
            input.display_name,
            Vec::new(),
        );

        Ok(feature)
    }
}

impl ToProtobuf for ProfileModel {
    type Output = ganymede::v2::Profile;

    fn to_protobuf(self, include_nested: bool) -> Self::Output {
        Self::Output {
            name: ProfileName::new(self.id).into(),
            display_name: self.display_name(),
            feature_profiles: self.feature_profiles().to_protobuf(include_nested),
        }
    }
}