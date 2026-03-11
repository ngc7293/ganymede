use tonic::{Request, Response, Status};
use uuid::uuid;

use crate::database::models::AtmosphereDataModel;
use crate::database::Database;
use crate::ganymede;
use crate::Result;

pub struct MeasurementsService {
    database: Database,
}

impl MeasurementsService {
    pub fn new(database: Database) -> Self {
        MeasurementsService { database }
    }
}

#[tonic::async_trait]
impl ganymede::v2::measurements_service_server::MeasurementsService for MeasurementsService {
    async fn get_measurements(
        &self,
        _: Request<ganymede::v2::GetMeasurementsRequest>,
    ) -> Result<Response<ganymede::v2::GetMeasurementsResponse>, Status> {
        let _ = uuid!("68ff6c2b-2393-47b9-b26d-6cf69b3d56e6");

        // TODO

        Err(Status::unimplemented("not yet implemented"))
    }

    async fn push_measurements(
        &self,
        request: Request<ganymede::v2::PushMeasurementsRequest>,
    ) -> Result<Response<()>, Status> {
        let domain_id = uuid!("68ff6c2b-2393-47b9-b26d-6cf69b3d56e6");
        let mut transaction = self.database.for_domain(domain_id).begin().await?;

        let payload = request.into_inner();
        let atmosphere_data: Vec<AtmosphereDataModel> = (&payload).try_into()?;

        log::debug!("Parsed {} valid atmospheric data points", atmosphere_data.len());

        transaction.insert_many_atmosphere_data(atmosphere_data).await?;
        transaction.commit().await?;

        Ok(Response::new(()))
    }
}
