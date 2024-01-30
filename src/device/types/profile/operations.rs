use crate::{device::database::DomainDatabase, error::Error};

use super::model::ProfileModel;

impl<'a> DomainDatabase<'a> {
    pub async fn fetch_all_profiles(&self) -> Result<Vec<ProfileModel>, Error> {
        let mut profiles =
            match sqlx::query_as::<_, ProfileModel>("SELECT id, display_name FROM profiles WHERE domain_id = $1")
                .bind(self.domain_id())
                .fetch_all(self.pool())
                .await
            {
                Ok(profile) => profile,
                Err(err) => match err {
                    _ => return Err(Error::DatabaseError(err.to_string())),
                },
            };
        
        for profile in profiles.iter_mut() {
            profile.set_feature_profiles(self.fetch_all_feature_profiles(&profile.id()).await?);
        }

        Ok(profiles)
    }

    pub async fn fetch_one_profile(&self, id: &uuid::Uuid) -> Result<ProfileModel, Error> {
        let mut profile = match sqlx::query_as::<_, ProfileModel>(
            "SELECT id, display_name FROM profiles WHERE domain_id = $1 AND id = $2",
        )
        .bind(self.domain_id())
        .bind(id)
        .fetch_one(self.pool())
        .await
        {
            Ok(profile) => profile,
            Err(err) => match err {
                sqlx::Error::RowNotFound => return Err(Error::NoSuchResourceError("Feature", id.clone())),
                _ => return Err(Error::DatabaseError(err.to_string())),
            },
        };

        let feature_profiles = self.fetch_all_feature_profiles(id).await?;
        profile.set_feature_profiles(feature_profiles);

        Ok(profile)
    }

    pub async fn insert_profile(&self, profile: ProfileModel) -> Result<ProfileModel, Error> {
        let (profile_id,) = sqlx::query_as::<_, (uuid::Uuid,)>(
            "INSERT INTO profiles (domain_id, display_name) VALUES ($1, $2) RETURNING id",
        )
        .bind(self.domain_id())
        .bind(profile.display_name())
        .fetch_one(self.pool())
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        for fp in profile.feature_profiles().iter_mut() {
            fp.set_profile_id(profile_id);
            self.insert_feature_profile(fp.clone()).await?;
        }

        self.fetch_one_profile(&profile_id).await
    }

    pub async fn update_profile(&self, profile: ProfileModel) -> Result<ProfileModel, Error> {
        let (profile_id,) = sqlx::query_as::<_, (uuid::Uuid,)>(
            "UPDATE profiles SET display_name = $3 WHERE domain_id = $1 AND id = $2 RETURNING id",
        )
        .bind(self.domain_id())
        .bind(profile.id())
        .bind(profile.display_name())
        .fetch_one(self.pool())
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        self.fetch_one_profile(&profile_id).await
    }

    pub async fn delete_profile(&self, id: &uuid::Uuid) -> Result<(), Error> {
        let result = sqlx::query("DELETE FROM profiles WHERE domain_id = $1 AND id = $2")
            .bind(self.domain_id())
            .bind(id)
            .execute(self.pool())
            .await
            .map_err(|err| Error::DatabaseError(err.to_string()))?;

        if result.rows_affected() == 0 {
            return Err(Error::NoSuchResourceError("Feature", id.clone()));
        }

        // Related FeatureProfiles are deleted by ON DELETE CASCADE relationship
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    type TestResult = Result<(), Box<dyn std::error::Error>>;

    async fn insert_test_domain(pool: &sqlx::PgPool) -> sqlx::Result<()> {
        sqlx::query("INSERT INTO domains VALUES ('00000000-0000-0000-0000-000000000000', 'Test Domain')")
            .execute(pool)
            .await?;
        Ok(())
    }

    #[sqlx::test]
    async fn test_insert_profile(pool: sqlx::PgPool) -> TestResult {
        let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

        insert_test_domain(&pool).await?;
        let profile = ProfileModel::new(uuid::Uuid::nil(), "A profile".into(), Vec::new());
        let result = database.insert_profile(profile).await.unwrap();

        assert_ne!(result.id(), uuid::Uuid::nil());
        Ok(())
    }
}
// //     fn create_test_config() -> ConfigModel {
// //         ConfigModel {
// //             config_id: uuid::Uuid::nil(),
// //             domain_id: uuid::Uuid::nil(),
// //             display_name: "config".to_string(),
// //             light_config: serde_json::json!({"luminaires": []}),
// //         }
// //     }

// //     fn create_test_device(config_id: uuid::Uuid) -> DeviceModel {
// //         DeviceModel {
// //             device_id: uuid::Uuid::nil(),
// //             display_name: "This is a device!".to_string(),
// //             mac: mac::Mac::try_from("00:00:00:00:00:00".to_string()).unwrap(),
// //             config_id: config_id,
// //             description: "This is a description".to_string(),
// //             timezone: "America/Montreal".to_string(),
// //         }
// //     }

// //     async fn insert_test_config(
// //         pool: &sqlx::PgPool,
// //     ) -> Result<uuid::Uuid, Box<dyn std::error::Error>> {
// //         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());
// //         Ok(database
// //             .insert_config(&create_test_config())
// //             .await
// //             .unwrap()
// //             .config_id)
// //     }

// //     #[sqlx::test]
// //     async fn test_insert_device(pool: sqlx::PgPool) -> TestResult {
// //         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

// //         insert_test_domain(&pool).await?;
// //         let device = create_test_device(insert_test_config(&pool).await?);

// //         let result = database.insert_device(device).await.unwrap();

// //         assert_ne!(result.device_id, uuid::Uuid::nil());
// //         Ok(())
// //     }

// //     #[sqlx::test]
// //     async fn test_insert_device_no_such_config(pool: sqlx::PgPool) -> TestResult {
// //         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

// //         insert_test_domain(&pool).await?;
// //         let device = create_test_device(uuid::Uuid::nil());

// //         let error = database.insert_device(device).await.unwrap_err();
// //         assert_eq!(error, DeviceError::NoSuchConfig);
// //         Ok(())
// //     }

// //     #[sqlx::test]
// //     async fn test_insert_device_duplicate_mac(pool: sqlx::PgPool) -> TestResult {
// //         env_logger::init();
// //         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

// //         insert_test_domain(&pool).await?;
// //         let device = create_test_device(insert_test_config(&pool).await?);

// //         let _ = database.insert_device(device.clone()).await;
// //         let error = database.insert_device(device).await.unwrap_err();

// //         assert_eq!(error, DeviceError::MacConflict);
// //         Ok(())
// //     }

// //     #[sqlx::test]
// //     async fn test_update_device(pool: sqlx::PgPool) -> TestResult {
// //         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

// //         insert_test_domain(&pool).await?;
// //         let device = create_test_device(insert_test_config(&pool).await?);

// //         let updated = DeviceModel {
// //             display_name: "Different device".to_string(),
// //             description: "Yup totally different".to_string(),
// //             ..database.insert_device(device).await.unwrap()
// //         };

// //         let result = database.update_device(updated).await;
// //         assert!(result.is_ok());
// //         Ok(())
// //     }

// //     #[sqlx::test]
// //     async fn test_update_device_no_such_config(pool: sqlx::PgPool) -> TestResult {
// //         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

// //         insert_test_domain(&pool).await?;
// //         let device = create_test_device(insert_test_config(&pool).await?);

// //         let updated = DeviceModel {
// //             config_id: uuid::Uuid::nil(),
// //             ..database.insert_device(device).await.unwrap()
// //         };

// //         let error = database.update_device(updated).await.unwrap_err();
// //         assert_eq!(error, DeviceError::NoSuchConfig);
// //         Ok(())
// //     }

// //     #[sqlx::test]
// //     async fn test_update_device_duplicate_mac(pool: sqlx::PgPool) -> TestResult {
// //         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

// //         insert_test_domain(&pool).await?;

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
