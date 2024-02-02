use tonic::{Request, Response, Status};

use crate::{
    auth::authenticate,
    device::{database::DomainDatabase, service::DeviceService, types::feature_profile::name::FeatureProfileName},
    ganymede,
};

use super::{model::ProfileModel, name::ProfileName};

impl DeviceService {
    pub async fn get_profile_inner(
        &self,
        request: Request<ganymede::v2::GetProfileRequest>,
    ) -> Result<Response<ganymede::v2::Profile>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();
        let profile_id = ProfileName::try_from(&payload.name)?.into();

        let profile = database.fetch_one_profile(&profile_id).await?;
        let feature_profiles = database.fetch_all_feature_profiles(&profile.id()).await?;

        Ok(Response::new(ganymede::v2::Profile {
            feature_profiles: feature_profiles.into_iter().map(|v| v.into()).collect(),
            ..profile.into()
        }))
    }

    pub async fn create_profile_inner(
        &self,
        request: Request<ganymede::v2::CreateProfileRequest>,
    ) -> Result<Response<ganymede::v2::Profile>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();

        let profile = match payload.profile.clone() {
            Some(profile) => ganymede::v2::Profile {
                name: ProfileName::nil().into(),
                feature_profiles: profile
                    .feature_profiles
                    .into_iter()
                    .map(|fp| ganymede::v2::FeatureProfile {
                        name: FeatureProfileName::nil(uuid::Uuid::nil()).into(),
                        ..fp
                    })
                    .collect(),
                ..profile
            },
            None => return Err(Status::invalid_argument("missing profile")),
        };

        let model: ProfileModel = profile.try_into()?;
        let result = database.insert_profile(model).await?;

        Ok(Response::new(result.into()))
    }

    pub async fn update_profile_inner(
        &self,
        request: Request<ganymede::v2::UpdateProfileRequest>,
    ) -> Result<Response<ganymede::v2::Profile>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();

        let profile = match payload.profile.clone() {
            Some(profile) => ganymede::v2::Profile {
                name: ProfileName::nil().into(),
                feature_profiles: profile
                    .feature_profiles
                    .into_iter()
                    .map(|fp| {
                        let mut name = fp.name;

                        if name.is_empty() {
                            name = FeatureProfileName::nil(uuid::Uuid::nil()).into()
                        }

                        ganymede::v2::FeatureProfile {
                            name: name,
                            ..fp
                        }
                    }
                    )
                    .collect(),
                ..profile
            },
            None => return Err(Status::invalid_argument("missing profile")),
        };

        let model: ProfileModel = profile.try_into()?;
        let result = database.update_profile(model).await?;
        let feature_profiles = database.fetch_all_feature_profiles(&result.id()).await?;

        Ok(Response::new(ganymede::v2::Profile {
            feature_profiles: feature_profiles.into_iter().map(|v| v.into()).collect(),
            ..result.into()
        }))
    }

    pub async fn delete_profile_inner(
        &self,
        request: Request<ganymede::v2::DeleteProfileRequest>,
    ) -> Result<Response<()>, Status> {
        let domain_id = authenticate(&request)?;
        let database = DomainDatabase::new(self.pool(), domain_id);

        let payload = request.into_inner();
        let profile_id = ProfileName::try_from(&payload.name)?.into();
        database.delete_profile(&profile_id).await?;
        Ok(Response::new(()))
    }
}