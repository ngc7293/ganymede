use crate::result::{Error, Result};
use tonic::transport::Server;

use crate::database::database::Database;
use crate::ganymede::v2::{
    device_service_server::DeviceServiceServer, measurements_service_server::MeasurementsServiceServer,
};
use crate::services::{DeviceService, MeasurementsService};

mod database;
mod ganymede;
mod result;
mod services;
mod types;

#[derive(clap::Parser, Debug)]
#[command(version, about, long_about = None)]
struct Arguments {
    // Path to the configuration file
    #[arg(short, long, default_value_t = String::from("Ganymede.toml"))]
    pub config: String,
}

#[derive(serde::Deserialize)]
struct RuntimeConfiguration {
    pub postgres_uri: String,
    pub port: u16,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    configure_logging();

    let configuration = try_read_configuration_file("Ganymede.toml")?;

    let database = Database::try_from_uri(&configuration.postgres_uri).await?;
    let device = DeviceService::new(database.clone());
    let measurements = MeasurementsService::new(database.clone());

    let addr = format!("0.0.0.0:{0}", configuration.port).parse()?;
    let reflection = tonic_reflection::server::Builder::configure()
        .register_encoded_file_descriptor_set(ganymede::v2::FILE_DESCRIPTOR_SET)
        .build_v1alpha()
        .map_err(|err| {
            log::error!("Failed to create reflection service: {err}");
            err
        })?;

    let (mut health_reporter, health_service) = tonic_health::server::health_reporter();

    let future = Server::builder()
        .add_service(reflection)
        .add_service(health_service)
        .add_service(DeviceServiceServer::new(device))
        .add_service(MeasurementsServiceServer::new(measurements))
        .serve(addr);

    health_reporter.set_serving::<DeviceServiceServer<DeviceService>>().await;
    health_reporter.set_serving::<MeasurementsServiceServer<MeasurementsService>>().await;
    future.await?;

    Ok(())
}

fn configure_logging() {
    let env = env_logger::Env::new().default_filter_or("INFO");
    env_logger::init_from_env(env);
}

fn try_read_configuration_file(path: &str) -> Result<RuntimeConfiguration, Box<dyn std::error::Error>> {
    let settings_file = std::fs::read_to_string(path).map_err(|err| {
        log::error!("Failed to read 'Ganymede.toml': {err}");
        err
    })?;

    let settings: RuntimeConfiguration = toml::from_str(&settings_file).map_err(|err| {
        log::error!("Failed to read 'Ganymede.toml': {err}");
        err
    })?;

    Ok(settings)
}
