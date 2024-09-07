use std::sync::Arc;

use crate::{Error, Result};

use super::transaction::DomainDatabaseTransaction;

#[derive(Clone)]
pub struct Database {
    connection_pool: Arc<sqlx::Pool<sqlx::Postgres>>,
}

impl Database {
    pub async fn try_from_uri(uri: &str) -> Result<Self> {
        let pool = sqlx::postgres::PgPoolOptions::new().max_connections(5).connect(uri).await.map_err(|err| {
            log::error!("Failed to connect to postgres: {err}");
            Error::DatabaseError(err.to_string())
        })?;

        log::info!("database connected");
        Ok(Database::new(pool))
    }

    pub fn new(connection_pool: sqlx::Pool<sqlx::Postgres>) -> Self {
        Database {
            connection_pool: Arc::new(connection_pool),
        }
    }

    pub fn for_domain(&self, domain_id: uuid::Uuid) -> DomainDatabase {
        DomainDatabase::new(self.connection_pool.clone(), domain_id)
    }
}

pub struct DomainDatabase {
    connection_pool: Arc<sqlx::Pool<sqlx::Postgres>>,
    domain_id: uuid::Uuid,
}

impl DomainDatabase {
    pub fn new(connection_pool: Arc<sqlx::Pool<sqlx::Postgres>>, domain_id: uuid::Uuid) -> Self {
        DomainDatabase {
            connection_pool,
            domain_id,
        }
    }

    pub async fn begin(&self) -> Result<DomainDatabaseTransaction> {
        match self.connection_pool.begin().await {
            Ok(tx) => Ok(DomainDatabaseTransaction::new(tx, self.domain_id.clone())),
            Err(err) => Err(Error::DatabaseError(err.to_string())),
        }
    }
}
