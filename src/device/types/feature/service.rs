use tonic::{Request, Response, Status};

use crate::{
    auth::authenticate,
    device::{database::DomainDatabase, service::DeviceService, types::model::DomainDatabaseModel},
    ganymede,
};

use super::{model::FeatureModel, name::FeatureName};




impl DeviceService {
    pub async fn get_feature_inner(
        &self,
        request: Request<ganymede::v2::GetFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        let domain_id = authenticate(&request)?;
        let mut tx = self.pool().begin().await.unwrap();

        let payload = request.into_inner();
        let feature_id = FeatureName::try_from(&payload.name)?.into();

        let result = FeatureModel::fetch_one(&mut *tx, feature_id, domain_id).await?;
        Ok(Response::new(result.into()))
    }

    pub async fn create_feature_inner(
        &self,
        request: Request<ganymede::v2::CreateFeatureRequest>,
    ) -> Result<Response<ganymede::v2::Feature>, Status> {
        let _domain_id = authenticate(&request)?;
        let mut tx = self.pool().begin().await.unwrap();

        let payload = request.into_inner();

        let feature = match payload.feature {
            Some(feature) => ganymede::v2::Feature {
                name: FeatureName::nil().into(),
                ..feature
            },
            None => return Err(Status::invalid_argument("missing feature")),
        };

        let mut model: FeatureModel = feature.try_into()?;
        model.create(&mut tx).await?;
        Ok(Response::new(model.into()))
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
