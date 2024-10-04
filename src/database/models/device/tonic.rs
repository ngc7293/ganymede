use uuid::Uuid;

use crate::ganymede;
use crate::{Error, Result};

use super::model::DeviceModel;

impl TryFrom<ganymede::v2::Device> for DeviceModel {
    type Error = Error;

    fn try_from(value: ganymede::v2::Device) -> Result<DeviceModel> {
        let device_id = match value.uid.as_str() {
            "" => Uuid::nil(),
            _ => Uuid::try_parse(&value.uid)?,
        };

        let result = DeviceModel {
            device_id,
            display_name: value.display_name,
            mac: value.mac.try_into()?,
            config_id: Uuid::try_parse(&value.config_uid)?,
            description: value.description,
            timezone: value.timezone.parse::<chrono_tz::Tz>()?.to_string(),
            uptime: None,
            last_poll: None,
        };

        Ok(result)
    }
}

impl TryFrom<DeviceModel> for ganymede::v2::Device {
    type Error = Error;

    fn try_from(value: DeviceModel) -> Result<ganymede::v2::Device> {
        let last_poll = match value.last_poll {
            Some(timestamp) => Some(prost_types::Timestamp {
                seconds: timestamp.timestamp(),
                nanos: 0,
            }), // FIXME: Handle full precision
            None => None,
        };

        let uptime = match value.uptime {
            Some(duration) => Some(prost_types::Duration {
                seconds: duration.num_seconds(),
                nanos: duration.subsec_nanos(),
            }),
            None => None,
        };

        let result = ganymede::v2::Device {
            uid: value.device_id.to_string(),
            mac: value.mac.into(),
            display_name: value.display_name,
            description: value.description,
            timezone: value.timezone,
            config_uid: value.config_id.to_string(),
            last_poll,
            uptime,
        };

        Ok(result)
    }
}

#[cfg(test)]
mod tests {
    use uuid::uuid;

    use crate::types::MacAddress;

    use super::*;

    #[test]
    fn test_to_model() {
        let device_uid = Uuid::new_v4();
        let config_uid = Uuid::new_v4();

        let device = ganymede::v2::Device {
            uid: device_uid.to_string(),
            mac: "AA:bb:CC:dd:EE:ff".to_string(),
            display_name: "I am a device".to_string(),
            description: "Watch how I pour".to_string(),
            timezone: "America/Caracas".to_string(),
            config_uid: config_uid.to_string(),
            last_poll: None,
            uptime: None,
        };

        let model = DeviceModel::try_from(device).unwrap();
        assert_eq!(model.device_id, device_uid);
        assert_eq!(model.config_id, config_uid);
        assert_eq!(model.mac, MacAddress::try_from("aa:bb:cc:dd:ee:ff").unwrap());
        assert_eq!(model.display_name, "I am a device".to_string());
        assert_eq!(model.description, "Watch how I pour".to_string());
        assert_eq!(model.timezone, "America/Caracas");
    }

    #[test]
    fn test_refuses_invalid_timezone() {
        let device = ganymede::v2::Device {
            uid: Uuid::nil().to_string(),
            mac: "00:00:00:00:00:00".to_string(),
            display_name: "".to_string(),
            description: "".to_string(),
            timezone: "Rohan/Edoras".to_string(),
            config_uid: Uuid::nil().to_string(),
            last_poll: None,
            uptime: None,
        };

        let error = DeviceModel::try_from(device).unwrap_err();
        assert_eq!(error, Error::BadTimezone);
    }

    #[test]
    fn test_refuses_invalid_device_uid() {
        let device = ganymede::v2::Device {
            uid: "not-a-uid".to_string(),
            mac: "00:00:00:00:00:00".to_string(),
            display_name: "".to_string(),
            description: "".to_string(),
            timezone: "America/Montreal".to_string(),
            config_uid: Uuid::nil().to_string(),
            last_poll: None,
            uptime: None,
        };

        let error = DeviceModel::try_from(device).unwrap_err();
        assert_eq!(error, Error::BadUuid);
    }

    #[test]
    fn test_accepts_empty_device_uid() {
        let device = ganymede::v2::Device {
            uid: "".to_string(),
            mac: "00:00:00:00:00:00".to_string(),
            display_name: "".to_string(),
            description: "".to_string(),
            timezone: "America/Montreal".to_string(),
            config_uid: Uuid::nil().to_string(),
            last_poll: None,
            uptime: None,
        };

        let device = DeviceModel::try_from(device).unwrap();
        assert_eq!(device.device_id, Uuid::nil());
    }

    #[test]
    fn test_refuses_invalid_config_uid() {
        let device = ganymede::v2::Device {
            uid: Uuid::nil().to_string(),
            mac: "00:00:00:00:00:00".to_string(),
            display_name: "".to_string(),
            description: "".to_string(),
            timezone: "America/Montreal".to_string(),
            config_uid: "not-a-uid".to_string(),
            last_poll: None,
            uptime: None,
        };

        let error = DeviceModel::try_from(device).unwrap_err();
        assert_eq!(error, Error::BadUuid);
    }

    #[test]
    fn test_refuses_invalid_mac() {
        let device = ganymede::v2::Device {
            uid: Uuid::nil().to_string(),
            mac: "".to_string(),
            display_name: "".to_string(),
            description: "".to_string(),
            timezone: "America/Montreal".to_string(),
            config_uid: Uuid::nil().to_string(),
            last_poll: None,
            uptime: None,
        };

        let error = DeviceModel::try_from(device).unwrap_err();
        assert_eq!(error, Error::BadMacAddress);
    }

    #[test]
    fn test_to_proto() {
        let device = DeviceModel {
            device_id: uuid!("00000000-0000-0000-0000-000000000001"),
            display_name: "I am a device".to_string(),
            mac: MacAddress::try_from("aa:bb:cc:dd:ee:ff".to_string()).unwrap(),
            config_id: uuid!("ffffffff-ffff-ffff-ffff-ffffffffffff"),
            description: "Short and stout".to_string(),
            timezone: "America/Caracas".to_string(),
            last_poll: None,
            uptime: None,
        };

        let result = ganymede::v2::Device::try_from(device).unwrap();
        assert_eq!(result.uid, "00000000-0000-0000-0000-000000000001");
        assert_eq!(result.mac, "aa:bb:cc:dd:ee:ff");
        assert_eq!(result.display_name, "I am a device");
        assert_eq!(result.description, "Short and stout");
        assert_eq!(result.timezone, "America/Caracas");
        assert_eq!(result.config_uid, "ffffffff-ffff-ffff-ffff-ffffffffffff");
    }
}
