use chrono::{Offset, TimeDelta, TimeZone};
use uuid::Uuid;

use tonic::{Request, Response, Status};

use crate::database::models::config::operations::ConfigFilter;
use crate::database::models::device::operations::DeviceFilter;
use crate::database::Database;
use crate::ganymede;
use crate::ganymede::v2::PollResponse;
use crate::result::{Error, Result};
use crate::types::MacAddress;

use crate::database::models::DeviceModel;

use super::auth::authenticate;

pub struct DeviceService {
    database: Database,
}

impl DeviceService {
    pub fn new(database: Database) -> Self {
        DeviceService { database }
    }
}

#[tonic::async_trait]
impl ganymede::v2::device_service_server::DeviceService for DeviceService {
    async fn create_device(
        &self,
        request: Request<ganymede::v2::CreateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();

        let device = match payload.device {
            Some(device) => device,
            None => return Err(Status::invalid_argument("Missing device")),
        };

        let model: DeviceModel = device.try_into()?;
        let result = transaction.insert_device(model).await?;
        let model = transaction.fetch_one_device(&result).await?;

        transaction.commit().await?;

        Ok(Response::new(
            model.try_into().map_err(|_| Status::internal("unhandled error"))?,
        ))
    }

    async fn update_device(
        &self,
        request: Request<ganymede::v2::UpdateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();

        let device = match payload.device {
            Some(device) => device,
            None => return Err(Status::invalid_argument("Missing device")),
        };

        let model = device.try_into()?;
        let result = transaction.update_device(model).await?;
        let model = transaction.fetch_one_device(&result).await?;

        transaction.commit().await?;
        Ok(Response::new(model.try_into()?))
    }

    async fn get_device(
        &self,
        request: Request<ganymede::v2::GetDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();

        let device_id = Uuid::try_parse(&payload.device_uid).map_err(|_| Error::BadUuid)?;
        let result = transaction.fetch_one_device(&device_id).await?;

        transaction.commit().await?;
        Ok(Response::new(result.try_into()?))
    }

    async fn list_device(
        &self,
        request: Request<ganymede::v2::ListDeviceRequest>,
    ) -> Result<Response<ganymede::v2::ListDeviceResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;
        let payload = request.into_inner();
        let filter = match payload.filter {
            Some(filter) => match filter {
                ganymede::v2::list_device_request::Filter::ConfigUid(config_id) => {
                    match uuid::Uuid::try_parse(&config_id) {
                        Ok(uuid) => DeviceFilter::ConfigId(uuid),
                        Err(_) => Err(Error::BadUuid)?,
                    }
                }
                ganymede::v2::list_device_request::Filter::NameFilter(name_filter) => {
                    DeviceFilter::NameFilter(name_filter)
                }
            },
            None => DeviceFilter::None,
        };

        let results = transaction.fetch_many_device(filter).await?;
        transaction.commit().await?;

        let response = ganymede::v2::ListDeviceResponse {
            devices: results
                .into_iter()
                .map(|result| result.try_into())
                .collect::<Result<Vec<ganymede::v2::Device>>>()?,
        };
        Ok(Response::new(response))
    }

    async fn delete_device(&self, request: Request<ganymede::v2::DeleteDeviceRequest>) -> Result<Response<()>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();

        let device_id = match uuid::Uuid::try_parse(&payload.device_uid) {
            Ok(device_id) => device_id,
            Err(_) => Err(Error::BadUuid)?,
        };

        transaction.delete_device(&device_id).await?;

        transaction.commit().await?;
        Ok(Response::new(()))
    }

    async fn poll(
        &self,
        request: Request<ganymede::v2::PollRequest>,
    ) -> Result<Response<ganymede::v2::PollResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();

        let mac = MacAddress::try_parse(&payload.device_mac)?;
        let device_id = match transaction.fetch_device_id_for_mac(&mac).await? {
            Some(device_id) => device_id,
            None => Err(Error::NoSuchDevice)?,
        };

        let uptime = match payload.uptime {
            Some(duration) => Some(TimeDelta::seconds(duration.seconds) + TimeDelta::nanoseconds(duration.nanos.into())),
            None => None,
        };
        transaction.update_device_uptime(&device_id, uptime).await?;

        let device = transaction.fetch_one_device(&device_id).await?;
        let config = transaction.fetch_one_config(&device.config_id).await?;


        let tz: chrono_tz::Tz = match device.timezone.parse() {
            Ok(tz) => tz,
            Err(err) => {
                log::error!("Failed to parse timezone: {err}");
                Err(Error::BadTimezone)?
            }
        };

        let offset_seconds =
            tz.offset_from_utc_datetime(&chrono::Utc::now().naive_utc()).fix().local_minus_utc() as i64;

        let response = PollResponse {
            device_uid: device.device_id.to_string(),
            device_display_name: device.display_name,
            config_uid: config.config_id.to_string(),
            config_display_name: config.display_name,
            timezone_offset_minutes: offset_seconds / 60,
            poll_period: Some(prost_types::Duration {
                seconds: config.poll_period.num_seconds(),
                nanos: config.poll_period.subsec_nanos(),
            }),
            light_config: Some(
                match serde_json::from_value::<ganymede::v2::LightConfig>(config.light_config) {
                    Ok(config) => config,
                    Err(err) => {
                        log::error!("error parsing JSON from DB: {err}");
                        return Err(Error::BadLightConfiguration)?;
                    }
                },
            ),
        };

        transaction.update_device_last_poll(&device.device_id, chrono::offset::Utc::now()).await?;

        transaction.commit().await?;
        Ok(Response::new(response))
    }

    async fn create_config(
        &self,
        request: Request<ganymede::v2::CreateConfigRequest>,
    ) -> Result<Response<ganymede::v2::Config>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();
        let config = match payload.config {
            Some(config) => config,
            None => return Err(Status::invalid_argument("Missing configuration")),
        };

        let model = config.try_into()?;
        let result = transaction.insert_config(model).await?;
        let model = transaction.fetch_one_config(&result).await?;

        transaction.commit().await?;
        Ok(Response::new(model.try_into()?))
    }

    async fn update_config(
        &self,
        request: Request<ganymede::v2::UpdateConfigRequest>,
    ) -> Result<Response<ganymede::v2::Config>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();
        let config = match payload.config {
            Some(config) => config,
            None => return Err(Status::invalid_argument("Missing configuration")),
        };

        let model = config.try_into()?;
        let result = transaction.update_config(model).await?;
        let model = transaction.fetch_one_config(&result).await?;

        transaction.commit().await?;
        Ok(Response::new(model.try_into()?))
    }

    async fn get_config(
        &self,
        request: Request<ganymede::v2::GetConfigRequest>,
    ) -> Result<Response<ganymede::v2::Config>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();
        let config_id = Uuid::try_parse(&payload.config_uid).map_err(|_| Error::BadUuid)?;

        let model = transaction.fetch_one_config(&config_id).await?;

        transaction.commit().await?;
        Ok(Response::new(model.try_into()?))
    }

    async fn list_config(
        &self,
        request: Request<ganymede::v2::ListConfigRequest>,
    ) -> Result<Response<ganymede::v2::ListConfigResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();
        let filter = match payload.name_filter.is_empty() {
            false => ConfigFilter::NameFilter(payload.name_filter),
            true => ConfigFilter::None,
        };

        let results = transaction.fetch_many_config(filter).await?;

        let response = ganymede::v2::ListConfigResponse {
            configs: results
                .into_iter()
                .map(|result| result.try_into())
                .collect::<Result<Vec<ganymede::v2::Config>>>()?,
        };

        transaction.commit().await?;
        Ok(Response::new(response))
    }

    async fn delete_config(&self, request: Request<ganymede::v2::DeleteConfigRequest>) -> Result<Response<()>, Status> {
        let domain_id = authenticate(&request)?;
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();
        let config_id = match uuid::Uuid::try_parse(&payload.config_uid) {
            Ok(config_id) => config_id,
            Err(_) => Err(Error::BadUuid)?,
        };

        transaction.delete_config(&config_id).await?;

        transaction.commit().await?;
        Ok(Response::new(()))
    }
}
