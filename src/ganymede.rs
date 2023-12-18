#![allow(non_snake_case)]

pub mod v2 {
    tonic::include_proto!("ganymede.v2");

    pub mod configurations {
        tonic::include_proto!("ganymede.v2.configurations");
    }

    pub(crate) const FILE_DESCRIPTOR_SET: &[u8] = tonic::include_file_descriptor_set!("ganymede.v2");
}
