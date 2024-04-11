use std::result::Result;

use chrono::{Offset, TimeZone};

use tonic::{Request, Response, Status};

use crate::auth::authenticate;
use crate::ganymede;
use crate::ganymede::v2::PollResponse;
use crate::types::mac;

use super::config::errors::ConfigError;
use super::database::DomainDatabase;
use super::device::errors::DeviceError;
use super::device::model::DeviceModel;
use super::device::operations::DeviceFilter;

pub struct DeviceService {
    dbpool: sqlx::Pool<sqlx::Postgres>,
}

impl DeviceService {
    pub fn new(pool: sqlx::Pool<sqlx::Postgres>) -> Self {
        DeviceService { dbpool: pool }
    }
}

#[tonic::async_trait]
impl ganymede::v2::device_service_server::DeviceService for DeviceService {
    async fn create_device(
        &self,
        request: Request<ganymede::v2::CreateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();

        let device = match payload.device {
            Some(device) => device,
            None => return Err(Status::invalid_argument("Missing device")),
        };

        let model: DeviceModel = device.try_into()?;
        let result = database.insert_device(model).await?;
        Ok(Response::new(
            result
                .try_into()
                .map_err(|_| Status::internal("unhandled error"))?,
        ))
    }

    async fn update_device(
        &self,
        request: Request<ganymede::v2::UpdateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();

        let device = match payload.device {
            Some(device) => device,
            None => return Err(Status::invalid_argument("Missing device")),
        };

        let model = device.try_into()?;
        let result = database.update_device(model).await?;
        Ok(Response::new(result.try_into()?))
    }

    async fn get_device(
        &self,
        request: Request<ganymede::v2::GetDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();

        let device_id = match uuid::Uuid::try_parse(&payload.device_uid) {
            Ok(uuid) => uuid,
            Err(_) => Err(DeviceError::InvalidDeviceId)?
        };

        let result = database.fetch_one_device(device_id).await?;
        Ok(Response::new(result.try_into()?))
    }

    async fn list_device(
        &self,
        request: Request<ganymede::v2::ListDeviceRequest>,
    ) -> Result<Response<ganymede::v2::ListDeviceResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();
        let filter = match payload.filter {
            Some(filter) => match filter {
                ganymede::v2::list_device_request::Filter::ConfigUid(config_id) => {
                    match uuid::Uuid::try_parse(&config_id) {
                        Ok(uuid) => DeviceFilter::ConfigId(uuid),
                        Err(_) => Err(DeviceError::InvalidConfigId)?,
                    }
                }
                ganymede::v2::list_device_request::Filter::NameFilter(name_filter) => {
                    DeviceFilter::NameFilter(name_filter)
                }
            },
            None => DeviceFilter::None,
        };

        let results = database.fetch_many_device(filter).await?;
        Ok(Response::new(ganymede::v2::ListDeviceResponse {
            devices: results
                .into_iter()
                .map(|result| result.try_into())
                .collect::<Result<Vec<ganymede::v2::Device>, DeviceError>>()?,
        }))
    }

    async fn delete_device(
        &self,
        request: Request<ganymede::v2::DeleteDeviceRequest>,
    ) -> Result<Response<()>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();

        let device_id = match uuid::Uuid::try_parse(&payload.device_uid) {
            Ok(device_id) => device_id,
            Err(_) => Err(DeviceError::InvalidDeviceId)?,
        };

        database.delete_device(&device_id).await?;
        Ok(Response::new(()))
    }

    async fn poll(
        &self,
        request: Request<ganymede::v2::PollRequest>,
    ) -> Result<Response<ganymede::v2::PollResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();

        let mac = mac::Mac::try_parse(&payload.device_mac).map_err(|_| DeviceError::InvalidMac)?;
        let device_id = match database.fetch_device_id_for_mac(&mac).await? {
            Some(device_id) => device_id,
            None => Err(DeviceError::NoSuchDevice)?
        };

        let device = database.fetch_one_device(device_id).await?;
        let config = database.fetch_one_config(&device.config_id).await?;

        let tz: chrono_tz::Tz = match device.timezone.parse() {
            Ok(tz) => tz,
            Err(err) => {
                log::error!("Failed to parse timezone: {err}");
                Err(DeviceError::InvalidTimezone)?
            }
        };

        let offset_seconds = tz
            .offset_from_utc_datetime(&chrono::Utc::now().naive_utc())
            .fix()
            .local_minus_utc() as i64;


        let response = PollResponse {
            uid: device.device_id.to_string(),
            display_name: device.display_name,
            timezone_offset_minutes: offset_seconds / 60,
            config: Some(config.try_into()?),
        };

        Ok(Response::new(response))
    }

    async fn create_config(
        &self,
        request: Request<ganymede::v2::CreateConfigRequest>,
    ) -> Result<Response<ganymede::v2::Config>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();
        let config = match payload.config {
            Some(config) => config,
            None => return Err(Status::invalid_argument("Missing configuration")),
        };

        let model = config.try_into()?;
        let result = database.insert_config(&model).await?;
        Ok(Response::new(result.try_into()?))
    }

    async fn update_config(
        &self,
        request: Request<ganymede::v2::UpdateConfigRequest>,
    ) -> Result<Response<ganymede::v2::Config>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();
        let config = match payload.config {
            Some(config) => config,
            None => return Err(Status::invalid_argument("Missing configuration")),
        };

        let model = config.try_into()?;
        let result = database.update_config(&model).await?;
        Ok(Response::new(result.try_into()?))
    }

    async fn get_config(
        &self,
        request: Request<ganymede::v2::GetConfigRequest>,
    ) -> Result<Response<ganymede::v2::Config>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();
        let config_id = match uuid::Uuid::try_parse(&payload.config_uid) {
            Ok(config_id) => config_id,
            Err(_) => Err(ConfigError::InvalidConfigId)?,
        };

        let result = database.fetch_one_config(&config_id).await?;
        Ok(Response::new(result.try_into()?))
    }

    async fn list_config(
        &self,
        request: Request<ganymede::v2::ListConfigRequest>,
    ) -> Result<Response<ganymede::v2::ListConfigResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();
        let name_filter = if payload.name_filter.is_empty() {
            None
        } else {
            Some(payload.name_filter)
        };

        let results = database.fetch_many_config(name_filter).await?;
        Ok(Response::new(ganymede::v2::ListConfigResponse {
            configs: results
                .into_iter()
                .map(|result| result.try_into())
                .collect::<Result<Vec<ganymede::v2::Config>, ConfigError>>()?,
        }))
    }

    async fn delete_config(
        &self,
        request: Request<ganymede::v2::DeleteConfigRequest>,
    ) -> Result<Response<()>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(&self.dbpool, domain_id);

        let payload = request.into_inner();
        let config_id = match uuid::Uuid::try_parse(&payload.config_uid) {
            Ok(config_id) => config_id,
            Err(_) => Err(ConfigError::InvalidConfigId)?,
        };

        database.delete_config(&config_id).await?;
        Ok(Response::new(()))
    }
}
