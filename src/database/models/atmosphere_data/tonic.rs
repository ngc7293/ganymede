use uuid::Uuid;

use crate::ganymede;
use crate::{Error, Result};

use super::model::AtmosphereDataModel;

impl TryFrom<&ganymede::v2::PushMeasurementsRequest> for Vec<AtmosphereDataModel> {
    type Error = Error;

    fn try_from(value: &ganymede::v2::PushMeasurementsRequest) -> Result<Vec<AtmosphereDataModel>> {
        let mut accumulator = Vec::new();

        for measurement in value.measurements.iter() {
            let device_id = Uuid::try_parse(&measurement.device_id)?;
            let timestamp = measurement.timestamp.ok_or(Error::BadTimestamp)?;
            let observed_on = chrono::DateTime::from_timestamp(
                timestamp.seconds,
                timestamp.nanos.try_into().map_err(|_| Error::BadTimestamp)?,
            )
            .ok_or(Error::BadTimestamp)?;

            if let Some(data) = measurement.atmosphere {
                let model = AtmosphereDataModel {
                    device_id,
                    observed_on,
                    temperature: data.temperature.try_into()?,
                    relative_humidity: data.relative_humidity.try_into()?,
                };
                accumulator.push(model);
            }
        }

        Ok(accumulator)
    }
}
