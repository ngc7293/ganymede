use tonic::{Request, Response, Status};

use crate::{
    auth::authenticate,
    device::{database::DomainDatabase, service::DeviceService},
    ganymede,
};

use super::{model::FeatureModel, name::FeatureName};

impl DeviceService {
    pub async fn list_feature_inner(
        &self,
        request: Request<ganymede::v2::ListFeatureRequest>,
    ) -> Result<Response<ganymede::v2::ListFeatureResponse>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let results = database.fetch_all_features().await?;
        Ok(Response::new(ganymede::v2::ListFeatureResponse {
            features: results.into_iter().map(|r| r.into()).collect(),
        }))
    }

    pub async fn get_feature_inner(
        &self,
        request: Request<ganymede::v2::GetFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();
        let feature_id = FeatureName::try_from(&payload.name)?.into();
        let result = database.fetch_one_feature(&feature_id).await?;
        Ok(Response::new(result.into()))
    }

    pub async fn create_feature_inner(
        &self,
        request: Request<ganymede::v2::CreateFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();

        let feature = match payload.feature {
            Some(feature) => ganymede::v2::Feature {
                name: format!("features/{}", uuid::Uuid::nil()),
                ..feature
            },
            None => return Err(Status::invalid_argument("missing feature")),
        };

        let model: FeatureModel = feature.try_into()?;
        let result = database.insert_feature(model).await?;
        Ok(Response::new(result.into()))
    }

    pub async fn update_feature_inner(
        &self,
        request: Request<ganymede::v2::UpdateFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();

        let feature = match payload.feature {
            Some(feature) => feature,
            None => return Err(Status::invalid_argument("missing feature")),
        };

        let model: FeatureModel = feature.try_into()?;
        let result = database.update_feature(model).await?;
        Ok(Response::new(result.into()))
    }

    pub async fn delete_feature_inner(
        &self,
        request: Request<ganymede::v2::DeleteFeatureRequest>,
    ) -> Result<Response<()>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();
        let feature_id = FeatureName::try_from(&payload.name)?.into();
        database.delete_feature(&feature_id).await?;
        Ok(Response::new(()))
    }
}
