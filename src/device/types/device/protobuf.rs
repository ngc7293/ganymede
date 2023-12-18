// use chrono::{Offset, TimeZone};

use crate::ganymede;

use super::{errors::DeviceError, model::DeviceModel};

impl TryFrom<ganymede::v2::Device> for DeviceModel {
    type Error = DeviceError;

    fn try_from(value: ganymede::v2::Device) -> Result<DeviceModel, DeviceError> {
        if value.timezone.parse::<chrono_tz::Tz>().is_err() {
            return Err(DeviceError::InvalidTimezone);
        };

        let device_id = match value.name.as_str() {
            "" => uuid::Uuid::nil(),
            _ => uuid::Uuid::try_parse(&value.name).map_err(|_| DeviceError::InvalidDeviceId)?,
        };

        let result = DeviceModel {
            device_id,
            display_name: value.display_name,
            mac: value.mac.try_into().map_err(|_| DeviceError::InvalidMac)?,
            device_type_id: uuid::Uuid::try_parse(&value.device_type_name)
                .map_err(|_| DeviceError::InvalidDeviceTypeId)?,
            description: value.description,
            timezone: value.timezone,
        };

        Ok(result)
    }
}

impl TryFrom<DeviceModel> for ganymede::v2::Device {
    type Error = DeviceError;

    fn try_from(value: DeviceModel) -> Result<ganymede::v2::Device, DeviceError> {
        // let tz: chrono_tz::Tz = match value.timezone.parse() {
        //     Ok(tz) => tz,
        //     Err(err) => {
        //         log::error!("Failed to parse timezone: {err}");
        //         return Err(DeviceError::InvalidTimezone);
        //     }
        // };

        // let _offset_seconds = tz
        //     .offset_from_utc_datetime(&chrono::Utc::now().naive_utc())
        //     .fix()
        //     .local_minus_utc() as i64;

        let result = ganymede::v2::Device {
            name: "devices/".to_owned() + &value.device_id.to_string(),
            mac: value.mac.try_into().map_err(|_| DeviceError::InvalidMac)?,
            display_name: value.display_name,
            description: value.description,
            timezone: value.timezone,
            device_type_name: "device_types/".to_owned() + &value.device_type_id.to_string(),
            device_type: None,
        };

        Ok(result)
    }
}

// #[cfg(test)]
// mod tests {
//     use crate::types::mac;

//     use super::*;

//     #[test]
//     fn test_to_model() {
//         let device_uid = uuid::Uuid::new_v4();
//         let config_uid = uuid::Uuid::new_v4();

//         let device = ganymede::v2::Device {
//             uid: device_uid.to_string(),
//             mac: "AA:bb:CC:dd:EE:ff".to_string(),
//             display_name: "I am a device".to_string(),
//             description: "Watch how I pour".to_string(),
//             timezone: "America/Caracas".to_string(),
//             timezone_offset_minutes: 900, // Must be ignored
//             config_uid: config_uid.to_string(),
//         };

//         let model = DeviceModel::try_from(device).unwrap();
//         assert_eq!(model.device_id, device_uid);
//         assert_eq!(model.config_id, config_uid);
//         assert_eq!(model.mac, mac::Mac::try_from("aa:bb:cc:dd:ee:ff").unwrap());
//         assert_eq!(model.display_name, "I am a device".to_string());
//         assert_eq!(model.description, "Watch how I pour".to_string());
//         assert_eq!(model.timezone, "America/Caracas");
//     }

//     #[test]
//     fn test_refuses_invalid_timezone() {
//         let device = ganymede::v2::Device {
//             uid:  uuid::Uuid::nil().to_string(),
//             mac: "00:00:00:00:00:00".to_string(),
//             display_name: "".to_string(),
//             description: "".to_string(),
//             timezone: "Rohan/Edoras".to_string(),
//             timezone_offset_minutes: 0,
//             config_uid:  uuid::Uuid::nil().to_string(),
//         };

//         let error = DeviceModel::try_from(device).unwrap_err();
//         assert_eq!(error, DeviceError::InvalidTimezone);
//     }

//     #[test]
//     fn test_refuses_invalid_device_uid() {
//         let device = ganymede::v2::Device {
//             uid: "not-a-uid".to_string(),
//             mac: "00:00:00:00:00:00".to_string(),
//             display_name: "".to_string(),
//             description: "".to_string(),
//             timezone: "America/Montreal".to_string(),
//             timezone_offset_minutes: 0,
//             config_uid:  uuid::Uuid::nil().to_string(),
//         };

//         let error = DeviceModel::try_from(device).unwrap_err();
//         assert_eq!(error, DeviceError::InvalidDeviceId);
//     }

//     #[test]
//     fn test_accepts_empty_device_uid() {
//         let device = ganymede::v2::Device {
//             uid: "".to_string(),
//             mac: "00:00:00:00:00:00".to_string(),
//             display_name: "".to_string(),
//             description: "".to_string(),
//             timezone: "America/Montreal".to_string(),
//             timezone_offset_minutes: 0,
//             config_uid:  uuid::Uuid::nil().to_string(),
//         };

//         let device = DeviceModel::try_from(device).unwrap();
//         assert_eq!(device.device_id, uuid::Uuid::nil());
//     }

//     #[test]
//     fn test_refuses_invalid_config_uid() {
//         let device = ganymede::v2::Device {
//             uid:  uuid::Uuid::nil().to_string(),
//             mac: "00:00:00:00:00:00".to_string(),
//             display_name: "".to_string(),
//             description: "".to_string(),
//             timezone: "America/Montreal".to_string(),
//             timezone_offset_minutes: 0,
//             config_uid: "not-a-uid".to_string(),
//         };

//         let error = DeviceModel::try_from(device).unwrap_err();
//         assert_eq!(error, DeviceError::InvalidConfigId);
//     }

//     #[test]
//     fn test_refuses_invalid_mac() {
//         let device = ganymede::v2::Device {
//             uid:  uuid::Uuid::nil().to_string(),
//             mac: "".to_string(),
//             display_name: "".to_string(),
//             description: "".to_string(),
//             timezone: "America/Montreal".to_string(),
//             timezone_offset_minutes: 0,
//             config_uid:  uuid::Uuid::nil().to_string(),
//         };

//         let error = DeviceModel::try_from(device).unwrap_err();
//         assert_eq!(error, DeviceError::InvalidMac);
//     }

//     #[test]
//     fn test_to_proto() {
//         let device = DeviceModel {
//             device_id: uuid::uuid!("00000000-0000-0000-0000-000000000001"),
//             display_name: "I am a device".to_string(),
//             mac: mac::Mac::try_from("aa:bb:cc:dd:ee:ff".to_string()).unwrap(),
//             config_id: uuid::uuid!("ffffffff-ffff-ffff-ffff-ffffffffffff"),
//             description: "Short and stout".to_string(),
//             timezone: "America/Caracas".to_string(),
//         };

//         let result = ganymede::v2::Device::try_from(device).unwrap();
//         assert_eq!(result.uid, "00000000-0000-0000-0000-000000000001");
//         assert_eq!(result.mac, "aa:bb:cc:dd:ee:ff");
//         assert_eq!(result.display_name, "I am a device");
//         assert_eq!(result.description, "Short and stout");
//         assert_eq!(result.timezone, "America/Caracas");
//         assert_eq!(result.timezone_offset_minutes, -4 * 60);
//         assert_eq!(result.config_uid, "ffffffff-ffff-ffff-ffff-ffffffffffff");
//     }

//     #[test]
//     fn test_computes_correct_timezone_offset() {
//         let mut device = DeviceModel {
//             device_id: uuid::Uuid::nil(),
//             display_name: "".to_string(),
//             mac: mac::Mac::try_from("00:00:00:00:00:00".to_string()).unwrap(),
//             config_id: uuid::Uuid::nil(),
//             description: "".to_string(),
//             timezone: "America/Caracas".to_string(),
//         };

//         let result = ganymede::v2::Device::try_from(device.clone()).unwrap();
//         assert_eq!(result.timezone_offset_minutes, -4 * 60);

//         device.timezone = "Asia/Shanghai".to_string();
//         let result = ganymede::v2::Device::try_from(device.clone()).unwrap();
//         assert_eq!(result.timezone_offset_minutes, 8 * 60);
//     }
// }
