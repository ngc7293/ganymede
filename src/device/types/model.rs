use async_trait::async_trait;
use sqlx::PgConnection;

use crate::error::Error;

#[async_trait]
pub trait DomainDatabaseModel
where Self: Sized
{
    type PkType;

    async fn create(&mut self, conn: &mut PgConnection) -> Result<(), Error>;
    async fn save(&mut self, conn: &mut PgConnection) -> Result<(), Error>;
    async fn fetch_one(conn: &mut PgConnection, pk: Self::PkType, domain_id: uuid::Uuid) -> Result<Self, Error>;
    async fn fetch_all(conn: &mut PgConnection, domain_id: uuid::Uuid) -> Result<Vec<Self>, Error>;
    async fn delete(conn: &mut PgConnection, pk: Self::PkType, domain_id: uuid::Uuid) -> Result<(), Error>;

}
