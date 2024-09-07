use crate::{Error, Result};

#[derive(Debug)]
pub struct DomainDatabaseTransaction {
    transaction: sqlx::Transaction<'static, sqlx::Postgres>,
    domain_id: uuid::Uuid,
}

impl DomainDatabaseTransaction {
    pub fn new(transaction: sqlx::Transaction<'static, sqlx::Postgres>, domain_id: uuid::Uuid) -> Self {
        DomainDatabaseTransaction { transaction, domain_id }
    }

    pub fn domain_id(&self) -> uuid::Uuid {
        self.domain_id.clone()
    }

    pub fn executor(&mut self) -> &mut sqlx::PgConnection {
        &mut *self.transaction
    }

    pub async fn commit(self) -> Result<()> {
        match self.transaction.commit().await {
            Ok(result) => Ok(result),
            Err(err) => Err(Error::DatabaseError(err.to_string())),
        }
    }

    #[allow(dead_code)]
    pub async fn rollback(self) -> Result<()> {
        match self.transaction.rollback().await {
            Ok(result) => Ok(result),
            Err(err) => Err(Error::DatabaseError(err.to_string())),
        }
    }
}
