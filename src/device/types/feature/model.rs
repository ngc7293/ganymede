use async_trait::async_trait;
use sqlx::{FromRow, PgConnection};

use crate::{device::types::{model::DomainDatabaseModel, protobuf::{ToProtobuf, TryFromProtobuf}}, error::Error, ganymede};

use super::name::FeatureName;

#[derive(Copy, Clone, Debug, PartialEq, sqlx::Type)]
#[sqlx(type_name = "feature_type", rename_all = "lowercase")]
pub enum FeatureType {
    Light,
}

#[derive(Debug, Clone, PartialEq, FromRow)]
pub struct FeatureModel {
    id: uuid::Uuid,
    domain_id: uuid::Uuid,
    display_name: String,
    feature_type: FeatureType,
}

impl FeatureModel {
    pub fn new(id: uuid::Uuid, domain_id: uuid::Uuid, display_name: String, feature_type: FeatureType) -> Self {
        return FeatureModel {
            id: id,
            domain_id: domain_id,
            display_name: display_name,
            feature_type: feature_type,
        };
    }

    pub fn id(&self) -> uuid::Uuid {
        self.id
    }

    pub fn domain_id(&self) -> uuid::Uuid {
        self.domain_id
    }

    pub fn display_name(&self) -> String {
        self.display_name.clone()
    }

    pub fn feature_type(&self) -> FeatureType {
        self.feature_type
    }
}

#[async_trait]
impl DomainDatabaseModel for FeatureModel {
    type PkType = uuid::Uuid;

    async fn create(&mut self, conn: &mut PgConnection) -> Result<(), Error>

    {
        let (feature_id,) = sqlx::query_as::<_, (uuid::Uuid,)>(
            "INSERT INTO features (domain_id, display_name, feature_types) VALUES ($1, $2, $3) RETURNING id",
        )
        .bind(&self.domain_id)
        .bind(&self.display_name)
        .bind(&self.feature_type)
        .fetch_one(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        self.id = feature_id;
        Ok(())
    }

    async fn save(&mut self, conn: &mut PgConnection) -> Result<(), Error>

    {
        let _result = sqlx::query(
            "UPDATE features SET display_name = $1, feature_types = $2 WHERE id = $3 AND domain_id = $4",
        )
        .bind(&self.display_name)
        .bind(&self.feature_type)
        .bind(&self.domain_id)
        .bind(&self.id)
        .fetch_one(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        Ok(())
    }

    async fn fetch_one(conn: &mut PgConnection, pk: Self::PkType, domain_id: uuid::Uuid) -> Result<Self, Error>

    {
        let feature = sqlx::query_as::<_, FeatureModel>(
            "SELECT id, domain_id, display_name, feature_type FROM features WHERE id = $1 AND domain_id = $2"
        )
        .bind(pk)
        .bind(domain_id)
        .fetch_one(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        Ok(feature)
    }

    async fn fetch_all(conn: &mut PgConnection, domain_id: uuid::Uuid) -> Result<Vec<Self>, Error>

    {
        let features = sqlx::query_as::<_, FeatureModel>(
            "SELECT id, domain_id, display_name, feature_type FROM features WHERE domain_id = $1"
        )
        .bind(domain_id)
        .fetch_all(&mut *conn)
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        Ok(features)
    }

    async fn delete(conn: &mut PgConnection, pk: Self::PkType, domain_id: uuid::Uuid) -> Result<(), Error>

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

impl TryFromProtobuf for FeatureModel {
    type Input = ganymede::v2::Feature;

    fn try_from_protobuf(input: Self::Input, domain_id: uuid::Uuid, strip_names: bool) -> Result<Self, Error> {
        let id = match strip_names {
            true => uuid::Uuid::nil(),
            false => FeatureName::try_from(&input.name)?.into(),
        };
        let feature_type = input.feature_type.try_into()?;

        let feature = FeatureModel::new(
            id,
            domain_id,
            input.display_name,
            feature_type,
        );

        Ok(feature)
    }
}

impl ToProtobuf for FeatureModel {
    type Output = ganymede::v2::Feature;

    fn to_protobuf(self, _include_nested: bool) -> Self::Output {
        Self::Output {
            name: FeatureName::new(self.id).into(),
            display_name: self.display_name,
            feature_type: ganymede::v2::FeatureType::from(self.feature_type).into(),
        }
    }
}