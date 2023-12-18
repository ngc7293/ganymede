use std::result::Result;

use tonic::{Request, Response, Status};

use crate::auth::authenticate;
use crate::ganymede;

pub struct MeasurementsService {}

impl MeasurementsService {
    pub fn new() -> Self {
        MeasurementsService {}
    }
}

#[tonic::async_trait]
impl ganymede::v2::measurements_service_server::MeasurementsService for MeasurementsService {
    async fn get_measurements(
        &self,
        request: Request<ganymede::v2::GetMeasurementsRequest>,
    ) -> Result<Response<ganymede::v2::GetMeasurementsResponse>, Status> {
        authenticate(&request)?;

        // TODO

        Err(Status::unimplemented("not yet implemented"))
    }

    async fn push_measurements(
        &self,
        request: Request<ganymede::v2::PushMeasurementsRequest>,
    ) -> Result<Response<()>, Status> {
        authenticate(&request)?;

        // TODO

        Err(Status::unimplemented("not yet implemented"))
    }
}
