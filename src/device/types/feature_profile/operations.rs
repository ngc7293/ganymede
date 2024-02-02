use uuid::Uuid;

use crate::{device::{database::DomainDatabase, types::feature::model::FeatureType}, error::Error};

use super::model::FeatureProfileModel;

impl<'a> DomainDatabase<'a> {
    pub async fn fetch_all_feature_profiles(&self, profile_id: &uuid::Uuid) -> Result<Vec<FeatureProfileModel>, Error> {
        let feature_profiles =
            match sqlx::query_as::<_, (Uuid, String, Uuid, Uuid, FeatureType, String)>("SELECT feature_profiles.id, feature_profiles.display_name, feature_profiles.profile_id, feature_profiles.feature_id, features.feature_type, feature_profiles.config::text FROM feature_profiles INNER JOIN features ON features.id = feature_profiles.feature_id WHERE features.domain_id = $1 AND feature_profiles.domain_id = $1 AND feature_profiles.profile_id = $2")
                .bind(self.domain_id())
                .bind(profile_id)
                .fetch_all(self.pool())
                .await
            {
                Ok(rows) => rows.into_iter().map(|(id, display_name, profile_id, feature_id, feature_type, config)| -> Result<FeatureProfileModel, Error> {
                    FeatureProfileModel::try_new(id, uuid::Uuid::nil(), display_name, profile_id, feature_id, feature_type, serde_json::from_str(&config).unwrap())
                }).collect::<Result<Vec<FeatureProfileModel>, Error>>()?,
                Err(err) => match err {
                    _ => return Err(Error::DatabaseError(err.to_string())),
                },
            };

        Ok(feature_profiles)
    }

    pub async fn fetch_one_feature_profile(&self, profile_id: &uuid::Uuid, id: &uuid::Uuid) -> Result<FeatureProfileModel, Error> {
        let feature_profile = match sqlx::query_as::<_, (Uuid, String, Uuid, Uuid, FeatureType, String)>(
            "SELECT feature_profiles.id, feature_profiles.display_name, profile_id, feature_id, feature_type, config::text FROM feature_profiles INNER JOIN features ON features.id = feature_profiles.feature_id WHERE features.domain_id = $1 AND feature_profiles.domain_id = $1 AND feature_profiles.profile_id = $2 AND feature_profiles.id = $3",
        )
        .bind(self.domain_id())
        .bind(profile_id)
        .bind(id)
        .fetch_one(self.pool())
        .await
        {
            Ok((id, display_name, profile_id, feature_id, feature_type, config)) => FeatureProfileModel::try_new(id, uuid::Uuid::nil(), display_name, profile_id, feature_id, feature_type, serde_json::from_str(&config).unwrap())?,
            Err(err) => match err {
                sqlx::Error::RowNotFound => return Err(Error::NoSuchResourceError("FeatureProfile", id.clone())),
                _ => return Err(Error::DatabaseError(err.to_string())),
            },
        };

        Ok(feature_profile)
    }

    pub async fn insert_feature_profile(&self, feature_profile: FeatureProfileModel) -> Result<FeatureProfileModel, Error> {
        let (feature_profile_id,) = sqlx::query_as::<_, (uuid::Uuid,)>(
            "INSERT INTO feature_profiles (domain_id, display_name, profile_id, feature_id, config) VALUES ($1, $2, $3, $4, $5::JSONB) RETURNING id",
        )
        .bind(self.domain_id())
        .bind(feature_profile.display_name())
        .bind(feature_profile.profile_id())
        .bind(feature_profile.feature_id())
        .bind(feature_profile.config().as_json()?)
        .fetch_one(self.pool())
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        self.fetch_one_feature_profile(&feature_profile.profile_id(), &feature_profile_id).await
    }

    pub async fn update_feature_profile(&self, feature_profile: FeatureProfileModel) -> Result<FeatureProfileModel, Error> {
        let (feature_profile_id,) = sqlx::query_as::<_, (uuid::Uuid,)>(
            "UPDATE profiles SET display_name = $4, profile_id = $5, feature_id = $6, config = $7 WHERE domain_id = $1 AND profile_id = $2 AND id = $3 RETURNING id",
        )
        .bind(self.domain_id())
        .bind(feature_profile.profile_id())
        .bind(feature_profile.id())
        .bind(feature_profile.profile_id())
        .bind(feature_profile.display_name())
        .bind(feature_profile.profile_id())
        .bind(feature_profile.feature_id())
        .bind(feature_profile.config().as_json()?)
        .fetch_one(self.pool())
        .await
        .map_err(|err| Error::DatabaseError(err.to_string()))?;

        self.fetch_one_feature_profile(&feature_profile.profile_id(), &feature_profile_id).await
    }

    pub async fn delete_feature_profile(&self, profile_id: &uuid::Uuid, id: &uuid::Uuid) -> Result<(), Error> {
        let result = sqlx::query("DELETE FROM feature_profiles WHERE domain_id = $1 AND  AND profile_id = $2 id = $3")
            .bind(self.domain_id())
            .bind(profile_id)
            .bind(id)
            .execute(self.pool())
            .await
            .map_err(|err| Error::DatabaseError(err.to_string()))?;

        if result.rows_affected() == 0 {
            return Err(Error::NoSuchResourceError("FeatureProfile", id.clone()));
        }

        Ok(())
    }
}

// #[cfg(test)]
// mod tests {
//     use super::*;

//     type TestResult = Result<(), Box<dyn std::error::Error>>;

//     async fn insert_test_domain(pool: &sqlx::PgPool) -> sqlx::Result<()> {
//         sqlx::query("INSERT INTO domains VALUES ('00000000-0000-0000-0000-000000000000', 'Test Domain')")
//             .execute(pool)
//             .await?;
//         Ok(())
//     }

//     #[sqlx::test]
//     async fn test_insert_feature_profile(pool: sqlx::PgPool) -> TestResult {
//         let database = DomainDatabase::new(&pool, uuid::Uuid::nil());

//         insert_test_domain(&pool).await?;
//         let profile = FeatureProfileModel::new(uuid::Uuid::nil(), "A profile".into());
//         let result = database.insert_feature_profile(profile).await.unwrap();

//         assert_ne!(result.id(), uuid::Uuid::nil());
//         Ok(())
//     }
// }
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
