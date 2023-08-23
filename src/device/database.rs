pub struct DomainDatabase<'a> {
    connection_pool: &'a sqlx::Pool<sqlx::Postgres>,
    domain_id: uuid::Uuid,
}

impl<'a> DomainDatabase<'a> {
    pub fn new(connection_pool: &'a sqlx::Pool<sqlx::Postgres>, domain_id: uuid::Uuid) -> Self {
        DomainDatabase {
            connection_pool: connection_pool,
            domain_id: domain_id,
        }
    }

    pub fn pool(&self) -> &'a sqlx::Pool<sqlx::Postgres> {
        &self.connection_pool
    }

    pub fn domain_id(&self) -> &uuid::Uuid {
        &self.domain_id
    }
}
