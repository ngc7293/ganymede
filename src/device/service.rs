use std::result::Result;

use tonic::{Request, Response, Status};

use crate::ganymede::{self, v2::PollDeviceResponse};

pub struct DeviceService {
    dbpool: sqlx::Pool<sqlx::Postgres>,
}

impl DeviceService {
    pub fn new(pool: sqlx::Pool<sqlx::Postgres>) -> Self {
        DeviceService { dbpool: pool }
    }

    pub fn pool(&self) -> &sqlx::Pool<sqlx::Postgres> {
        &self.dbpool
    }
}

#[tonic::async_trait]
impl ganymede::v2::device_service_server::DeviceService for DeviceService {
    async fn list_feature(
        &self,
        request: Request<ganymede::v2::ListFeatureRequest>,
    ) -> Result<Response<ganymede::v2::ListFeatureResponse>, Status> {
        self.list_feature_inner(request).await
    }

    async fn get_feature(
        &self,
        request: Request<ganymede::v2::GetFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        self.get_feature_inner(request).await
    }

    async fn create_feature(
        &self,
        request: Request<ganymede::v2::CreateFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        self.create_feature_inner(request).await
    }

    async fn update_feature(
        &self,
        request: Request<ganymede::v2::UpdateFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        self.update_feature_inner(request).await
    }

    async fn delete_feature(
        &self,
        request: Request<ganymede::v2::DeleteFeatureRequest>,
    ) -> Result<Response<()>, Status> {
        self.delete_feature_inner(request).await
    }

    async fn list_profile(
        &self,
        request: Request<ganymede::v2::ListProfileRequest>,
    ) -> Result<Response<ganymede::v2::ListProfileResponse>, Status> {
        self.list_profile_inner(request).await
    }

    async fn get_profile(
        &self,
        request: Request<ganymede::v2::GetProfileRequest>,
    ) -> Result<Response<ganymede::v2::Profile>, Status> {
        self.get_profile_inner(request).await
    }

    async fn create_profile(
        &self,
        request: Request<ganymede::v2::CreateProfileRequest>,
    ) -> Result<Response<ganymede::v2::Profile>, Status> {
        self.create_profile_inner(request).await
    }

    async fn update_profile(
        &self,
        request: Request<ganymede::v2::UpdateProfileRequest>,
    ) -> Result<Response<ganymede::v2::Profile>, Status> {
        self.update_profile_inner(request).await
    }

    async fn delete_profile(
        &self,
        request: Request<ganymede::v2::DeleteProfileRequest>,
    ) -> Result<Response<()>, Status> {
        self.delete_profile_inner(request).await
    }

    async fn list_device_type(
        &self,
        _request: Request<ganymede::v2::ListDeviceTypeRequest>,
    ) -> Result<Response<ganymede::v2::ListDeviceTypeResponse>, Status> {
        Err(tonic::Status::unimplemented("unimplemented"))
    }

    async fn get_device_type(
        &self,
        _request: Request<ganymede::v2::GetDeviceTypeRequest>,
    ) -> Result<Response<ganymede::v2::DeviceType>, Status> {
        Err(tonic::Status::unimplemented("unimplemented"))
    }

    async fn create_device_type(
        &self,
        _request: Request<ganymede::v2::CreateDeviceTypeRequest>,
    ) -> Result<Response<ganymede::v2::DeviceType>, Status> {
        Err(tonic::Status::unimplemented("unimplemented"))
    }

    async fn update_device_type(
        &self,
        _request: Request<ganymede::v2::UpdateDeviceTypeRequest>,
    ) -> Result<Response<ganymede::v2::DeviceType>, Status> {
        Err(tonic::Status::unimplemented("unimplemented"))
    }

    async fn delete_device_type(
        &self,
        _request: Request<ganymede::v2::DeleteDeviceTypeRequest>,
    ) -> Result<Response<()>, Status> {
        Err(tonic::Status::unimplemented("unimplemented"))
    }

    async fn list_device(
        &self,
        request: Request<ganymede::v2::ListDeviceRequest>,
    ) -> Result<Response<ganymede::v2::ListDeviceResponse>, Status> {
        self.list_device_internal(request).await
    }

    async fn get_device(
        &self,
        request: Request<ganymede::v2::GetDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        self.get_device_internal(request).await
    }

    async fn create_device(
        &self,
        request: Request<ganymede::v2::CreateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        self.create_device_internal(request).await
    }

    async fn update_device(
        &self,
        request: Request<ganymede::v2::UpdateDeviceRequest>,
    ) -> Result<Response<ganymede::v2::Device>, Status> {
        self.update_device_internal(request).await
    }

    async fn delete_device(&self, request: Request<ganymede::v2::DeleteDeviceRequest>) -> Result<Response<()>, Status> {
        self.delete_device_internal(request).await
    }

    async fn poll_device(
        &self,
        _request: Request<ganymede::v2::PollDeviceRequest>,
    ) -> Result<Response<PollDeviceResponse>, Status> {
        Err(tonic::Status::unimplemented("unimplemented"))
    }
}
