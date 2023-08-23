use std::{env, path::PathBuf};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let descriptor_path = PathBuf::from(env::var("OUT_DIR").unwrap()).join("ganymede.v2.bin");

    tonic_build::configure()
        .build_client(false)
        .build_server(true)
        .file_descriptor_set_path(descriptor_path)
        .type_attribute(".", "#[derive(serde::Serialize, serde::Deserialize)]")
        .type_attribute(".", "#[serde(rename_all = \"camelCase\")]")
        .compile(&["api/ganymede/v2/device.proto"], &["api/"])?;
    Ok(())
}
