use crate::device::database::DomainDatabase;

use super::{errors::ConfigError, model::ConfigModel};

impl<'a> DomainDatabase<'a> {
    pub async fn exists_config(&self, config_id: &uuid::Uuid) -> Result<bool, ConfigError> {
        let (row,) = sqlx::query_as::<_, (bool,)>(
            "SELECT EXISTS (SELECT 1 FROM config WHERE domain_id = $1 AND config_id = $2)",
        )
        .bind(self.domain_id())
        .bind(config_id)
        .fetch_one(self.pool())
        .await
        .map_err(|err| ConfigError::DatabaseError(err.to_string()))?;
        Ok(row)
    }

    pub async fn insert_config(&self, config: &ConfigModel) -> Result<ConfigModel, ConfigError> {
        let row = sqlx::query_as::<_, (uuid::Uuid,)>("INSERT INTO config(domain_id, display_name, light_config) VALUES ($1, $2, $3) RETURNING config_id")
            .bind(self.domain_id())
            .bind(&config.display_name)
            .bind(&config.light_config)
            .fetch_one(self.pool())
            .await
            .map_err(|err| ConfigError::DatabaseError(err.to_string()))?;

        self.fetch_one_config(&row.0).await
    }

    pub async fn update_config(&self, config: &ConfigModel) -> Result<ConfigModel, ConfigError> {
        let row = match sqlx::query_as::<_, (uuid::Uuid,)>("UPDATE config SET display_name = $3, light_config = $4 WHERE domain_id = $1 AND config_id = $2 RETURNING config_id")
            .bind(self.domain_id())
            .bind(&config.config_id)
            .bind(&config.display_name)
            .bind(&config.light_config)
            .fetch_one(self.pool())
            .await
        {
            Ok(row) => row,
            Err(err) => match err {
                sqlx::Error::RowNotFound => return Err(ConfigError::NoSuchConfig),
                _ => return Err(ConfigError::DatabaseError(err.to_string())),
            },
        };

        self.fetch_one_config(&row.0).await
    }

    pub async fn fetch_one_config(
        &self,
        config_id: &uuid::Uuid,
    ) -> Result<ConfigModel, ConfigError> {
        let row = match sqlx::query_as::<_, ConfigModel>("SELECT config_id, domain_id, display_name, light_config FROM config WHERE domain_id = $1 AND config_id = $2")
            .bind(self.domain_id())
            .bind(config_id)
            .fetch_one(self.pool())
            .await
        {
            Ok(row) => row,
            Err(err) => match err {
                sqlx::Error::RowNotFound => return Err(ConfigError::NoSuchConfig),
                _ => return Err(ConfigError::DatabaseError(err.to_string())),
            },
        };

        Ok(row)
    }

    pub async fn fetch_many_config(
        &self,
        name_filter: Option<String>,
    ) -> Result<Vec<ConfigModel>, ConfigError> {
        let mut query = sqlx::QueryBuilder::new("SELECT config_id, domain_id, display_name, light_config FROM config WHERE domain_id = ");
        query.push_bind(self.domain_id());

        if let Some(name_filter) = name_filter {
            query
                .push(" AND display_name LIKE ")
                .push_bind(format!("%{name_filter}%"));
        }

        let rows = query
            .build_query_as::<ConfigModel>()
            .fetch_all(self.pool())
            .await
            .map_err(|err| ConfigError::DatabaseError(err.to_string()))?;
        Ok(rows)
    }

    pub async fn delete_config(&self, config_id: &uuid::Uuid) -> Result<(), ConfigError> {
        let (is_used,) = sqlx::query_as::<_, (bool,)>("SELECT EXISTS(SELECT 1 FROM device WHERE config_id = $1)")
            .bind(config_id)
            .fetch_one(self.pool())
            .await
            .map_err(|err| ConfigError::DatabaseError(err.to_string()))?;

        if is_used {
            return Err(ConfigError::ConfigInUse);
        }

        let result = sqlx::query("DELETE FROM config WHERE domain_id = $1 AND config_id = $2")
            .bind(self.domain_id())
            .bind(config_id)
            .execute(self.pool())
            .await
            .map_err(|err| ConfigError::DatabaseError(err.to_string()))?;

        match result.rows_affected() {
            0 => Err(ConfigError::NoSuchConfig),
            _ => Ok(()),
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::{device::device::model::DeviceModel, types::mac};

    use super::*;

    async fn insert_test_domain(pool: &sqlx::PgPool) -> sqlx::Result<()> {
        sqlx::query(
            "INSERT INTO domain VALUES ('00000000-0000-0000-0000-000000000000', 'root', '')",
        )
        .execute(pool)
        .await?;
        Ok(())
    }

    fn create_test_config() -> ConfigModel {
        ConfigModel {
            config_id: uuid::Uuid::nil(),
            domain_id: uuid::Uuid::nil(),
            display_name: "config".to_string(),
            light_config: serde_json::json!({"luminaires": []}),
        }
    }

    fn create_test_device(config_id: uuid::Uuid) -> DeviceModel {
        DeviceModel {
            device_id: uuid::Uuid::nil(),
            display_name: "This is a device!".to_string(),
            mac: mac::Mac::try_from("00:00:00:00:00:00".to_string()).unwrap(),
            config_id: config_id,
            description: "This is a description".to_string(),
            timezone: "America/Montreal".to_string(),
        }
    }

    #[sqlx::test]
    async fn test_can_remove_config_in_use(pool: sqlx::PgPool) {
        let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

        insert_test_domain(&pool).await.unwrap();

        let config = database.insert_config(&create_test_config()).await.unwrap();
        let _ = database.insert_device(create_test_device(config.config_id.clone())).await.unwrap();

        let err = database.delete_config(&config.config_id).await.unwrap_err();
        assert_eq!(err, ConfigError::ConfigInUse);
    }
}