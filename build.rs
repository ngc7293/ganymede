use std::{env, path::PathBuf};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let descriptor_path = PathBuf::from(env::var("OUT_DIR").unwrap()).join("ganymede.v2.bin");

    tonic_build::configure()
        .build_client(false)
        .build_server(true)
        .file_descriptor_set_path(descriptor_path)
        .type_attribute(
            "ganymede.v2.LightConfig",
            "#[derive(serde::Serialize, serde::Deserialize)] #[serde(rename_all = \"camelCase\")]",
        )
        .type_attribute(
            "ganymede.v2.Luminaire",
            "#[derive(serde::Serialize, serde::Deserialize)] #[serde(rename_all = \"camelCase\")]",
        )
        .type_attribute(
            "ganymede.v2.Luminaire.DailySchedule",
            "#[derive(serde::Serialize, serde::Deserialize)] #[serde(rename_all = \"camelCase\")]",
        )
        .type_attribute(
            "ganymede.v2.Time",
            "#[derive(serde::Serialize, serde::Deserialize)] #[serde(rename_all = \"camelCase\")]",
        )
        .compile(
            &[
                "api/ganymede/v2/device.proto",
                "api/ganymede/v2/measurements.proto",
            ],
            &["api/"],
        )?;
    Ok(())
}
