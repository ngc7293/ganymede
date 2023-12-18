use tonic::{Request, Response, Status};

use crate::device::service::DeviceService;

use crate::auth::authenticate;
use crate::ganymede;
// use crate::ganymede::v2::Device;
// use crate::types::mac::Mac;
use crate::device::database::DomainDatabase;

use super::errors::DeviceError;
use super::model::DeviceModel;
use super::operations::{DeviceFilter, UniqueDeviceFilter};

impl DeviceService {
    pub async fn create_device_internal(
        &self,
        request: Request<ganymede::v2::CreateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();

        let device = match payload.device {
            Some(device) => device,
            None => return Err(Status::invalid_argument("Missing device")),
        };

        let model: DeviceModel = device.try_into()?;
        let result = database.insert_device(model).await?;
        Ok(Response::new(
            result.try_into().map_err(|_| Status::internal("unhandled error"))?,
        ))
    }

    pub async fn update_device_internal(
        &self,
        request: Request<ganymede::v2::UpdateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();

        let device = match payload.device {
            Some(device) => device,
            None => return Err(Status::invalid_argument("Missing device")),
        };

        let model = device.try_into()?;
        let result = database.update_device(model).await?;
        Ok(Response::new(result.try_into()?))
    }

    pub async fn get_device_internal(
        &self,
        request: Request<ganymede::v2::GetDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        // let payload = request.into_inner();

        let result = database
            .fetch_one_device(UniqueDeviceFilter::DeviceId(uuid::Uuid::nil()))
            .await?;
        Ok(Response::new(result.try_into()?))
    }

    pub async fn list_device_internal(
        &self,
        request: Request<ganymede::v2::ListDeviceRequest>,
    ) -> Result<Response<ganymede::v2::ListDeviceResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        // let payload = request.into_inner();

        let results = database
            .fetch_many_device(DeviceFilter::NameFilter("".to_owned()))
            .await?;
        Ok(Response::new(ganymede::v2::ListDeviceResponse {
            devices: results
                .into_iter()
                .map(|result| result.try_into())
                .collect::<Result<Vec<ganymede::v2::Device>, DeviceError>>()?,
        }))
    }

    pub async fn delete_device_internal(
        &self,
        request: Request<ganymede::v2::DeleteDeviceRequest>,
    ) -> Result<Response<()>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();

        let device_id = match uuid::Uuid::try_parse(&payload.name) {
            Ok(device_id) => device_id,
            Err(_) => Err(DeviceError::InvalidDeviceId)?,
        };

        database.delete_device(&device_id).await?;
        Ok(Response::new(()))
    }
}
