use sqlx::postgres::PgPoolOptions;
use tonic::transport::Server;

use crate::device::service::DeviceService;
use ganymede::v2::device_service_server::DeviceServiceServer;

use crate::measurements::service::MeasurementsService;
use ganymede::v2::measurements_service_server::MeasurementsServiceServer;

pub mod auth;
pub mod device;
pub mod error;
pub mod ganymede;
pub mod measurements;
pub mod name;
pub mod types;

#[derive(serde::Deserialize)]
struct Settings {
    pub postgres_uri: String,
    pub port: u16,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    env_logger::init();

    let settings_file = std::fs::read_to_string("Ganymede.toml").map_err(|err| {
        log::error!("Failed to read 'Ganymede.toml': {err}");
        err
    })?;

    let settings: Settings = toml::from_str(&settings_file).map_err(|err| {
        log::error!("Failed to parse 'Ganymede.toml': {err}");
        err
    })?;

    let postgres = PgPoolOptions::new()
        .max_connections(5)
        .connect(&settings.postgres_uri)
        .await
        .map_err(|err| {
            log::error!("Failed to connect to postgres: {err}");
            err
        })?;

    let addr = format!("0.0.0.0:{0}", settings.port).parse()?;
    let reflection = tonic_reflection::server::Builder::configure()
        .register_encoded_file_descriptor_set(ganymede::v2::FILE_DESCRIPTOR_SET)
        .build()
        .map_err(|err| {
            log::error!("Failed to create reflection service: {err}");
            err
        })?;

    let device = DeviceService::new(postgres);
    let measurements = MeasurementsService::new();

    Server::builder()
        .add_service(reflection)
        .add_service(DeviceServiceServer::new(device))
        .add_service(MeasurementsServiceServer::new(measurements))
        .serve(addr)
        .await?;

    Ok(())
}
