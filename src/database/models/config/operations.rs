use uuid::Uuid;

use crate::database::DomainDatabaseTransaction;
use crate::{Error, Result};

use super::model::ConfigModel;

pub enum ConfigFilter {
    NameFilter(String),
    None,
}

impl DomainDatabaseTransaction {
    pub async fn is_config_in_use(&mut self, config_id: &Uuid) -> Result<bool> {
        let result = sqlx::query_as::<_, (bool,)>(
            "SELECT EXISTS(
                SELECT 1
                FROM device
                WHERE config_id = $1
            )",
        )
        .bind(config_id)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(row) => Ok(row.0),
            Err(err) => Err(err.into()),
        }
    }

    pub async fn fetch_one_config(&mut self, config_id: &uuid::Uuid) -> Result<ConfigModel> {
        let result = sqlx::query_as::<_, ConfigModel>(
            "SELECT
                config_id, domain_id, display_name, poll_period, light_config
            FROM config
            WHERE
                domain_id = $1
                AND config_id = $2",
        )
        .bind(self.domain_id())
        .bind(config_id)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(row) => Ok(row),
            Err(err) => match err {
                sqlx::Error::RowNotFound => Err(Error::NoSuchConfig),
                _ => return Err(err.into()),
            },
        }
    }

    pub async fn fetch_many_config(&mut self, filter: ConfigFilter) -> Result<Vec<ConfigModel>> {
        let mut query = sqlx::QueryBuilder::new(
            "SELECT
                config_id, domain_id, display_name, poll_period, light_config
            FROM config
            WHERE domain_id = ",
        );
        query.push_bind(self.domain_id());

        match filter {
            ConfigFilter::NameFilter(name_filter) => {
                query.push(" AND display_name LIKE ").push_bind(format!("%{name_filter}%"));
            }
            ConfigFilter::None => (),
        }

        let result = query.build_query_as::<ConfigModel>().fetch_all(self.executor()).await;

        match result {
            Ok(rows) => Ok(rows),
            Err(err) => Err(err.into()),
        }
    }

    pub async fn insert_config(&mut self, config: ConfigModel) -> Result<Uuid> {
        let result = sqlx::query_as::<_, (uuid::Uuid,)>(
            "INSERT INTO config(
                domain_id, display_name, poll_period, light_config
            ) VALUES (
                $1, $2, $3, $4
            ) RETURNING config_id",
        )
        .bind(self.domain_id())
        .bind(&config.display_name)
        .bind(&config.poll_period)
        .bind(&config.light_config)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(row) => Ok(row.0),
            Err(err) => Err(err.into()),
        }
    }

    pub async fn update_config(&mut self, config: ConfigModel) -> Result<Uuid> {
        let result = sqlx::query_as::<_, (uuid::Uuid,)>(
            "UPDATE config
            SET
                display_name = $3,
                poll_period = $4,
                light_config = $5
            WHERE
                domain_id = $1
                AND config_id = $2
            RETURNING config_id",
        )
        .bind(self.domain_id())
        .bind(&config.config_id)
        .bind(&config.display_name)
        .bind(&config.poll_period)
        .bind(&config.light_config)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(row) => Ok(row.0),
            Err(sqlx::Error::RowNotFound) => Err(Error::NoSuchConfig),
            Err(err) => Err(err.into()),
        }
    }

    pub async fn delete_config(&mut self, config_id: &uuid::Uuid) -> Result<()> {
        if self.is_config_in_use(config_id).await? {
            return Err(Error::ConfigInUse);
        }

        let result = sqlx::query(
            "DELETE FROM config
            WHERE
                domain_id = $1
                AND config_id = $2",
        )
        .bind(self.domain_id())
        .bind(config_id)
        .execute(self.executor())
        .await;

        match result {
            Ok(row) => match row.rows_affected() {
                1 => Ok(()),
                0 => Err(Error::NoSuchDevice),
                n => Err(Error::DatabaseError(format!(
                    "Update segment affected {n} but we expected 1"
                ))),
            },
            Err(err) => Err(err.into()),
        }
    }
}

#[cfg(test)]
mod tests {
    use chrono::TimeDelta;
    use uuid::uuid;

    use crate::database::{Database, DomainDatabaseTransaction};

    use super::*;

    type TestResult = Result<(), Box<dyn std::error::Error>>;

    async fn start_transaction(pool: sqlx::PgPool) -> DomainDatabaseTransaction {
        Database::new(pool).for_domain(Uuid::nil()).begin().await.expect("Failed to acquire transaction")
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config")))]
    async fn test_read_config(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let result = transaction.fetch_one_config(&Uuid::nil()).await.unwrap();

        assert_eq!(result.display_name, "Test Config");
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain")))]
    async fn test_insert_config(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let config = ConfigModel {
            config_id: Uuid::nil(),
            display_name: "config".to_string(),
            poll_period: TimeDelta::hours(1),
            light_config: serde_json::json!({"luminaires": []}),
        };

        let config_id = transaction.insert_config(config).await.unwrap();
        assert_ne!(config_id, Uuid::nil());
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain")))]
    async fn test_read_many_config(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let configs = vec![
            ConfigModel {
                config_id: uuid!("00000000-0000-0000-0000-000000000001"),
                display_name: "config-1".to_string(),
                poll_period: TimeDelta::hours(1),
                light_config: serde_json::json!({"luminaires": []}),
            },
            ConfigModel {
                config_id: uuid!("00000000-0000-0000-0000-000000000002"),
                display_name: "config-2".to_string(),
                poll_period: TimeDelta::hours(1),
                light_config: serde_json::json!({"luminaires": []}),
            },
        ];

        for config in configs.iter() {
            transaction.insert_config(config.clone()).await?;
        }

        let result = transaction.fetch_many_config(ConfigFilter::None).await?;
        assert_eq!(result.len(), 2);

        let result = transaction.fetch_many_config(ConfigFilter::NameFilter("config-1".into())).await?;
        assert_eq!(result.len(), 1);

        let result = transaction.fetch_many_config(ConfigFilter::NameFilter("device-1".into())).await?;
        assert_eq!(result.len(), 0);

        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config")))]
    async fn test_update_config(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let updated = ConfigModel {
            config_id: Uuid::nil(),
            display_name: "Different config".to_string(),
            poll_period: chrono::TimeDelta::seconds(600),
            light_config: serde_json::json!({"luminaires": []}),
        };

        transaction.update_config(updated.clone()).await.unwrap();
        let read = transaction.fetch_one_config(&Uuid::nil()).await.unwrap();

        assert_eq!(updated, read);
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config", "device")))]
    async fn test_can_remove_config_in_use(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let err = transaction.delete_config(&Uuid::nil()).await.unwrap_err();
        assert_eq!(err, Error::ConfigInUse);
        Ok(())
    }
}
