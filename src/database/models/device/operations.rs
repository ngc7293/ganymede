use chrono::{DateTime, TimeDelta, Utc};
use uuid::Uuid;

use crate::database::DomainDatabaseTransaction;
use crate::types::MacAddress;
use crate::{Error, Result};

use super::model::DeviceModel;

pub enum DeviceFilter {
    NameFilter(String),
    ConfigId(uuid::Uuid),
    None,
}

impl DomainDatabaseTransaction {
    pub async fn fetch_device_id_for_mac(&mut self, mac: &MacAddress) -> Result<Option<Uuid>> {
        let result = sqlx::query_as::<_, (uuid::Uuid,)>(
            "SELECT
                device_id
            FROM device
            WHERE
                domain_id = $1
                AND mac = $2
            LIMIT 1",
        )
        .bind(self.domain_id())
        .bind(mac)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(row) => Ok(Some(row.0)),
            Err(err) => match err {
                sqlx::Error::RowNotFound => Ok(None),
                _ => return Err(err.into()),
            },
        }
    }

    pub async fn fetch_one_device(&mut self, device_id: &Uuid) -> Result<DeviceModel> {
        let result = sqlx::query_as::<_, DeviceModel>(
            "SELECT
                device_id, domain_id, display_name, mac, config_id, description, timezone, last_poll, uptime
            FROM device
            WHERE
                domain_id = $1
                AND device_id = $2",
        )
        .bind(self.domain_id())
        .bind(device_id)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(model) => Ok(model),
            Err(err) => match err {
                sqlx::Error::RowNotFound => Err(Error::NoSuchDevice),
                _ => Err(err.into()),
            },
        }
    }

    pub async fn fetch_many_device(&mut self, filter: DeviceFilter) -> Result<Vec<DeviceModel>> {
        let mut query = sqlx::QueryBuilder::new(
            "SELECT
                device_id, domain_id, display_name, mac, config_id, description, timezone, last_poll, uptime
            FROM device
            WHERE domain_id =",
        );
        query.push_bind(self.domain_id());

        match filter {
            DeviceFilter::NameFilter(name_filter) => {
                query.push(" AND display_name LIKE ").push_bind(format!("%{name_filter}%"));
            }
            DeviceFilter::ConfigId(config_id) => {
                query.push(" AND config_id = ").push_bind(config_id);
            }
            DeviceFilter::None => (),
        };

        let result = query.build_query_as::<DeviceModel>().fetch_all(self.executor()).await;

        match result {
            Ok(rows) => Ok(rows),
            Err(err) => Err(err.into()),
        }
    }

    pub async fn insert_device(&mut self, device: DeviceModel) -> Result<Uuid> {
        let result = sqlx::query_as::<_, (uuid::Uuid,)>(
            "INSERT INTO device(
                domain_id, display_name, mac, config_id, description, timezone
            ) VALUES (
                $1, $2, $3, $4, $5, $6
            ) RETURNING device_id",
        )
        .bind(self.domain_id())
        .bind(device.display_name)
        .bind(device.mac)
        .bind(device.config_id)
        .bind(device.description)
        .bind(device.timezone)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(row) => Ok(row.0),
            Err(err) => match err.as_database_error() {
                Some(db_err) => match db_err.constraint() {
                    Some("device_config_id_fkey") => Err(Error::NoSuchConfig),
                    Some("device_mac_unique") => Err(Error::DuplicateMacAddress),
                    _ => Err(Error::DatabaseError(err.to_string())),
                },
                None => Err(Error::DatabaseError(err.to_string())),
            },
        }
    }

    pub async fn update_device(&mut self, device: DeviceModel) -> Result<Uuid> {
        let result = sqlx::query_as::<_, (uuid::Uuid,)>(
            "UPDATE device
            SET
                display_name = $3,
                mac = $4,
                config_id = $5,
                description = $6,
                timezone = $7
            WHERE
                domain_id = $1
                AND device_id = $2
            RETURNING device_id",
        )
        .bind(self.domain_id())
        .bind(&device.device_id)
        .bind(&device.display_name)
        .bind(&device.mac)
        .bind(&device.config_id)
        .bind(&device.description)
        .bind(&device.timezone)
        .fetch_one(self.executor())
        .await;

        match result {
            Ok(row) => Ok(row.0),
            Err(sqlx::Error::RowNotFound) => Err(Error::NoSuchDevice),
            Err(err) => match err.as_database_error() {
                Some(db_err) => match db_err.constraint() {
                    Some("device_config_id_fkey") => Err(Error::NoSuchConfig),
                    Some("device_mac_unique") => Err(Error::DuplicateMacAddress),
                    _ => Err(Error::DatabaseError(err.to_string())),
                },
                None => Err(Error::DatabaseError(err.to_string())),
            },
        }
    }

    pub async fn delete_device(&mut self, device_id: &uuid::Uuid) -> Result<()> {
        let result = sqlx::query(
            "DELETE FROM device
            WHERE
                domain_id = $1
                AND device_id = $2",
        )
        .bind(self.domain_id())
        .bind(device_id)
        .execute(self.executor())
        .await;

        match result {
            Ok(row) => match row.rows_affected() {
                1 => Ok(()),
                0 => Err(Error::NoSuchDevice),
                n => Err(Error::DatabaseError(format!(
                    "Delete statement affected {n} but we expected 1"
                ))),
            },
            Err(err) => Err(err.into()),
        }
    }

    pub async fn update_device_last_poll(&mut self, device_id: &uuid::Uuid, last_poll: DateTime<Utc>) -> Result<()> {
        let result = sqlx::query(
            "
            UPDATE device
            SET
                last_poll = $3
            WHERE
                domain_id = $1
                AND device_id = $2",
        )
        .bind(self.domain_id())
        .bind(device_id)
        .bind(last_poll)
        .execute(self.executor())
        .await;

        match result {
            Ok(row) => match row.rows_affected() {
                1 => Ok(()),
                0 => Err(Error::NoSuchDevice),
                n => Err(Error::DatabaseError(format!(
                    "Update statement affected {n} but we expected 1"
                ))),
            },
            Err(err) => Err(err.into()),
        }
    }

    pub async fn update_device_uptime(&mut self, device_id: &uuid::Uuid, uptime: Option<TimeDelta>) -> Result<()> {
        let result = sqlx::query(
            "
            UPDATE device
            SET
                uptime = $3
            WHERE
                domain_id = $1
                AND device_id = $2",
        )
        .bind(self.domain_id())
        .bind(device_id)
        .bind(uptime)
        .execute(self.executor())
        .await;

        match result {
            Ok(row) => match row.rows_affected() {
                1 => Ok(()),
                0 => Err(Error::NoSuchDevice),
                n => Err(Error::DatabaseError(format!(
                    "Update statement affected {n} but we expected 1"
                ))),
            },
            Err(err) => Err(err.into()),
        }
    }
}

#[cfg(test)]
mod tests {
    use uuid::uuid;

    use crate::database::Database;
    use crate::types::MacAddress;

    use super::*;

    type TestResult = Result<(), Box<dyn std::error::Error>>;

    async fn start_transaction(pool: sqlx::PgPool) -> DomainDatabaseTransaction {
        Database::new(pool).for_domain(Uuid::nil()).begin().await.expect("Failed to acquire transaction")
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config")))]
    async fn test_insert_device(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let device = DeviceModel {
            device_id: Uuid::nil(),
            display_name: "This is a device!".to_string(),
            mac: MacAddress::try_from("00:00:00:00:00:00".to_string()).unwrap(),
            config_id: Uuid::nil(),
            description: "This is a description".to_string(),
            timezone: "America/Montreal".to_string(),
            last_poll: None,
            uptime: None,
        };

        let result = transaction.insert_device(device).await.unwrap();
        assert_ne!(result, Uuid::nil());
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain")))]
    async fn test_insert_device_no_such_config(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        // Same definition as before, but we haven't loaded the config fixture
        let device = DeviceModel {
            device_id: Uuid::nil(),
            display_name: "This is a device!".to_string(),
            mac: MacAddress::try_from("00:00:00:00:00:00".to_string()).unwrap(),
            config_id: Uuid::nil(),
            description: "This is a description".to_string(),
            timezone: "America/Montreal".to_string(),
            last_poll: None,
            uptime: None,
        };

        let result = transaction.insert_device(device).await.unwrap_err();
        assert_eq!(result, Error::NoSuchConfig);
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config", "device")))]
    async fn test_insert_device_duplicate_mac(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        // Same definition as before, but we already use the MAC address in the device fixture
        let device = DeviceModel {
            device_id: Uuid::nil(),
            display_name: "This is a device!".to_string(),
            mac: MacAddress::try_from("00:00:00:00:00:00".to_string()).unwrap(),
            config_id: Uuid::nil(),
            description: "This is a description".to_string(),
            timezone: "America/Montreal".to_string(),
            last_poll: None,
            uptime: None,
        };

        let result = transaction.insert_device(device).await.unwrap_err();
        assert_eq!(result, Error::DuplicateMacAddress);
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config", "device")))]
    async fn test_update_device(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let updated = DeviceModel {
            device_id: Uuid::nil(),
            display_name: "Different device".to_string(),
            description: "Yup totally different".to_string(),
            mac: MacAddress::try_from("00:00:00:00:00:00".to_string()).unwrap(),
            config_id: Uuid::nil(),
            timezone: "America/Montreal".to_string(),
            last_poll: None,
            uptime: None,
        };

        let returned = transaction.update_device(updated.clone()).await.unwrap();
        let read = transaction.fetch_one_device(&returned).await.unwrap();

        assert_eq!(updated, read);
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config", "device")))]
    async fn test_update_device_no_such_config(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let updated = DeviceModel {
            device_id: Uuid::nil(),
            display_name: "Different device".to_string(),
            description: "Yup totally different".to_string(),
            mac: MacAddress::try_from("00:00:00:00:00:00".to_string()).unwrap(),
            config_id: uuid!("00000000-0000-0000-0000-000000000001"),
            timezone: "America/Montreal".to_string(),
            last_poll: None,
            uptime: None,
        };

        let result = transaction.update_device(updated).await.unwrap_err();
        assert_eq!(result, Error::NoSuchConfig);
        Ok(())
    }

    #[sqlx::test(fixtures(path = "../../../../fixtures", scripts("domain", "config", "device")))]
    async fn test_update_device_duplicate_mac(pool: sqlx::PgPool) -> TestResult {
        let mut transaction = start_transaction(pool).await;

        let mut device = DeviceModel {
            device_id: Uuid::nil(),
            display_name: "Different device".to_string(),
            description: "Yup totally different".to_string(),
            mac: MacAddress::try_from("00:00:00:00:00:01".to_string())?,
            config_id: Uuid::nil(),
            timezone: "America/Montreal".to_string(),
            last_poll: None,
            uptime: None,
        };

        device.device_id = transaction.insert_device(device.clone()).await.unwrap();
        device.mac = MacAddress::try_from("00:00:00:00:00:00".to_string())?;

        let result = transaction.update_device(device).await.unwrap_err();
        assert_eq!(result, Error::DuplicateMacAddress);
        Ok(())
    }
}
