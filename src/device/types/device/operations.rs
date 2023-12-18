use crate::device::database::DomainDatabase;
use crate::types::mac::Mac;

use super::{errors::DeviceError, model::DeviceModel};

pub enum UniqueDeviceFilter {
    DeviceId(uuid::Uuid),
    DeviceMac(Mac),
}

pub enum DeviceFilter {
    NameFilter(String),
    // ConfigId(uuid::Uuid),
    // None,
}

impl<'a> DomainDatabase<'a> {
    pub async fn insert_device(&self, device: DeviceModel) -> Result<DeviceModel, DeviceError> {
        if self
            .fetch_one_device(UniqueDeviceFilter::DeviceMac(device.mac.clone()))
            .await
            != Err(DeviceError::NoSuchDevice)
        {
            return Err(DeviceError::MacConflict);
        }

        // match self.exists_config(&device.device_type_uid).await {
        //     Ok(true) => (),
        //     Ok(false) => return Err(DeviceError::NoSuchConfig),
        //     Err(ConfigError::DatabaseError(err)) => return Err(DeviceError::DatabaseError(err)),
        //     _ => return Err(DeviceError::DatabaseError("unknown error".to_string())),
        // };

        let (device_id,) = sqlx::query_as::<_, (uuid::Uuid,)>("INSERT INTO device(domain_id, display_name, mac, config_id, description, timezone) VALUES ($1, $2, $3, $4, $5, $6) RETURNING device_id")
            .bind(self.domain_id())
            .bind(device.display_name)
            .bind(device.mac)
            .bind(device.device_type_id)
            .bind(device.description)
            .bind(device.timezone)
            .fetch_one(self.pool())
            .await
            .map_err(|err| DeviceError::DatabaseError(err.to_string()))?;

        self.fetch_one_device(UniqueDeviceFilter::DeviceId(device_id)).await
    }

    pub async fn update_device(&self, device: DeviceModel) -> Result<DeviceModel, DeviceError> {
        if let Ok(existing) = self
            .fetch_one_device(UniqueDeviceFilter::DeviceMac(device.mac.clone()))
            .await
        {
            if existing.device_id != device.device_id {
                return Err(DeviceError::MacConflict);
            }
        }

        // match self.exists_config(&device.config_id).await {
        //     Ok(true) => (),
        //     Ok(false) => return Err(DeviceError::NoSuchConfig),
        //     Err(ConfigError::DatabaseError(err)) => return Err(DeviceError::DatabaseError(err)),
        //     _ => return Err(DeviceError::DatabaseError("unknown error".to_string())),
        // };

        let (device_id,) = match sqlx::query_as::<_, (uuid::Uuid,)>("UPDATE device SET display_name = $3, mac = $4, config_id = $5, description = $6, timezone = $7 WHERE domain_id = $1 AND device_id = $2 RETURNING device_id")
            .bind(self.domain_id())
            .bind(device.device_id)
            .bind(device.display_name)
            .bind(device.mac)
            .bind(device.device_type_id)
            .bind(device.description)
            .bind(device.timezone)
            .fetch_one(self.pool())
            .await
        {
            Ok(row) => row,
            Err(err) => match err {
                sqlx::Error::RowNotFound => return Err(DeviceError::NoSuchDevice),
                _ => return Err(DeviceError::DatabaseError(err.to_string())),
            },
        };

        self.fetch_one_device(UniqueDeviceFilter::DeviceId(device_id)).await
    }

    pub async fn fetch_one_device(&self, filter: UniqueDeviceFilter) -> Result<DeviceModel, DeviceError> {
        let mut query = sqlx::QueryBuilder::new("SELECT device_id, domain_id, display_name, mac, config_id, description, timezone FROM device WHERE domain_id = ");
        query.push_bind(self.domain_id()).push(" AND ");

        match filter {
            UniqueDeviceFilter::DeviceId(id) => query.push("device_id = ").push_bind(id),
            UniqueDeviceFilter::DeviceMac(mac) => query.push("mac = ").push_bind(mac),
        };

        let device = match query.build_query_as::<DeviceModel>().fetch_one(self.pool()).await {
            Ok(device) => device,
            Err(err) => match err {
                sqlx::Error::RowNotFound => return Err(DeviceError::NoSuchDevice),
                _ => return Err(DeviceError::DatabaseError(err.to_string())),
            },
        };

        Ok(device)
    }

    pub async fn fetch_many_device(&self, filter: DeviceFilter) -> Result<Vec<DeviceModel>, DeviceError> {
        let mut query = sqlx::QueryBuilder::new("SELECT device_id, domain_id, display_name, mac, config_id, description, timezone FROM device WHERE domain_id = ");
        query.push_bind(self.domain_id());

        match filter {
            DeviceFilter::NameFilter(name_filter) => {
                query
                    .push(" AND display_name LIKE ")
                    .push_bind(format!("%{name_filter}%"));
            }
            // DeviceFilter::ConfigId(config_id) => {
            //     query.push(" AND config_id = ").push_bind(config_id);
            // }
            // DeviceFilter::None => (),
        };

        let devices = query
            .build_query_as::<DeviceModel>()
            .fetch_all(self.pool())
            .await
            .map_err(|err| DeviceError::DatabaseError(err.to_string()))?;

        Ok(devices)
    }

    pub async fn delete_device(&self, device_id: &uuid::Uuid) -> Result<(), DeviceError> {
        let result = sqlx::query("DELETE FROM device WHERE domain_id = $1 AND device_id = $2")
            .bind(self.domain_id())
            .bind(device_id)
            .execute(self.pool())
            .await
            .map_err(|err| DeviceError::DatabaseError(err.to_string()))?;

        if result.rows_affected() == 0 {
            return Err(DeviceError::NoSuchDevice);
        }

        Ok(())
    }
}

// #[cfg(test)]
// mod tests {
//     use crate::{device::config::model::ConfigModel, types::mac};

//     use super::*;

//     type TestResult = Result<(), Box<dyn std::error::Error>>;

//     async fn insert_test_domain(pool: &sqlx::PgPool) -> sqlx::Result<()> {
//         sqlx::query(
//             "INSERT INTO domain VALUES ('00000000-0000-0000-0000-000000000000', 'root', '')",
//         )
//         .execute(pool)
//         .await?;
//         Ok(())
//     }

//     fn create_test_config() -> ConfigModel {
//         ConfigModel {
//             config_id: uuid::Uuid::nil(),
//             domain_id: uuid::Uuid::nil(),
//             display_name: "config".to_string(),
//             light_config: serde_json::json!({"luminaires": []}),
//         }
//     }

//     fn create_test_device(config_id: uuid::Uuid) -> DeviceModel {
//         DeviceModel {
//             device_id: uuid::Uuid::nil(),
//             display_name: "This is a device!".to_string(),
//             mac: mac::Mac::try_from("00:00:00:00:00:00".to_string()).unwrap(),
//             config_id: config_id,
//             description: "This is a description".to_string(),
//             timezone: "America/Montreal".to_string(),
//         }
//     }

//     async fn insert_test_config(
//         pool: &sqlx::PgPool,
//     ) -> Result<uuid::Uuid, Box<dyn std::error::Error>> {
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());
//         Ok(database
//             .insert_config(&create_test_config())
//             .await
//             .unwrap()
//             .config_id)
//     }

//     #[sqlx::test]
//     async fn test_insert_device(pool: sqlx::PgPool) -> TestResult {
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

//         insert_test_domain(&pool).await?;
//         let device = create_test_device(insert_test_config(&pool).await?);

//         let result = database.insert_device(device).await.unwrap();

//         assert_ne!(result.device_id, uuid::Uuid::nil());
//         Ok(())
//     }

//     #[sqlx::test]
//     async fn test_insert_device_no_such_config(pool: sqlx::PgPool) -> TestResult {
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

//         insert_test_domain(&pool).await?;
//         let device = create_test_device(uuid::Uuid::nil());

//         let error = database.insert_device(device).await.unwrap_err();
//         assert_eq!(error, DeviceError::NoSuchConfig);
//         Ok(())
//     }

//     #[sqlx::test]
//     async fn test_insert_device_duplicate_mac(pool: sqlx::PgPool) -> TestResult {
//         env_logger::init();
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

//         insert_test_domain(&pool).await?;
//         let device = create_test_device(insert_test_config(&pool).await?);

//         let _ = database.insert_device(device.clone()).await;
//         let error = database.insert_device(device).await.unwrap_err();

//         assert_eq!(error, DeviceError::MacConflict);
//         Ok(())
//     }

//     #[sqlx::test]
//     async fn test_update_device(pool: sqlx::PgPool) -> TestResult {
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

//         insert_test_domain(&pool).await?;
//         let device = create_test_device(insert_test_config(&pool).await?);

//         let updated = DeviceModel {
//             display_name: "Different device".to_string(),
//             description: "Yup totally different".to_string(),
//             ..database.insert_device(device).await.unwrap()
//         };

//         let result = database.update_device(updated).await;
//         assert!(result.is_ok());
//         Ok(())
//     }

//     #[sqlx::test]
//     async fn test_update_device_no_such_config(pool: sqlx::PgPool) -> TestResult {
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

//         insert_test_domain(&pool).await?;
//         let device = create_test_device(insert_test_config(&pool).await?);

//         let updated = DeviceModel {
//             config_id: uuid::Uuid::nil(),
//             ..database.insert_device(device).await.unwrap()
//         };

//         let error = database.update_device(updated).await.unwrap_err();
//         assert_eq!(error, DeviceError::NoSuchConfig);
//         Ok(())
//     }

//     #[sqlx::test]
//     async fn test_update_device_duplicate_mac(pool: sqlx::PgPool) -> TestResult {
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

//         insert_test_domain(&pool).await?;

//         let original = create_test_device(insert_test_config(&pool).await?);
//         let _ = database.insert_device(original).await.unwrap();

//         let device = DeviceModel {
//             mac: mac::Mac::try_from("aa:bb:cc:dd:ee:ff".to_string()).unwrap(),
//             ..create_test_device(insert_test_config(&pool).await?)
//         };

//         let updated = DeviceModel {
//             mac: mac::Mac::try_from("00:00:00:00:00:00".to_string()).unwrap(), // Already taken by original
//             ..database.insert_device(device).await.unwrap()
//         };

//         let error = database.update_device(updated).await.unwrap_err();
//         assert_eq!(error, DeviceError::MacConflict);
//         Ok(())
//     }
// }
