use crate::{Error, Result};

use crate::database::DomainDatabaseTransaction;

use super::model::AtmosphereDataModel;

impl DomainDatabaseTransaction {
    pub async fn insert_many_atmosphere_data(&mut self, atmosphere_data: Vec<AtmosphereDataModel>) -> Result<()> {
        let observed_on_list: Vec<chrono::DateTime<chrono::Utc>> =
            atmosphere_data.iter().map(|a| a.observed_on).collect();
        let domain_id_list: Vec<uuid::Uuid> = atmosphere_data.iter().map(|_| self.domain_id()).collect();
        let device_id_list: Vec<uuid::Uuid> = atmosphere_data.iter().map(|a| a.device_id).collect();
        let temperature_list: Vec<f32> = atmosphere_data.iter().map(|a| a.temperature.into()).collect();
        let relative_humidity_list: Vec<f32> = atmosphere_data.iter().map(|a| a.relative_humidity.into()).collect();

        let result = sqlx::query(
            "INSERT INTO atmosphere (
                observed_on, domain_id, device_id, temperature, relative_humidity
            ) SELECT * FROM UNNEST(
                $1::timestamp[], $2::UUID[], $3::UUID[], $4::FLOAT[], $5::FLOAT[]
            )",
        )
        .bind(observed_on_list)
        .bind(domain_id_list)
        .bind(device_id_list)
        .bind(temperature_list)
        .bind(relative_humidity_list)
        .execute(self.executor())
        .await;

        match result {
            Ok(_) => Ok(()),
            Err(err) => Err(Error::DatabaseError(err.to_string())),
        }
    }
}
