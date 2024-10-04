use crate::ganymede;
use crate::{Error, Result};

use super::model::ConfigModel;

impl TryFrom<ganymede::v2::Config> for ConfigModel {
    type Error = Error;

    fn try_from(value: ganymede::v2::Config) -> Result<ConfigModel> {
        let config_id = match value.uid.as_str() {
            "" => uuid::Uuid::nil(),
            _ => uuid::Uuid::try_parse(&value.uid)?,
        };

        let parsed_poll_period = match value.poll_period {
            Some(poll_period) => {
                chrono::TimeDelta::seconds(poll_period.seconds)
                    + chrono::TimeDelta::nanoseconds(poll_period.nanos.into())
            }
            None => return Err(Error::BadPollPeriod),
        };

        if parsed_poll_period < chrono::TimeDelta::minutes(10) {
            return Err(Error::BadPollPeriod);
        }

        let parsed_light_config = match value.light_config {
            Some(light_config) => match serde_json::to_value(&light_config) {
                Ok(json) => json,
                Err(_) => {
                    return Err(Error::BadLightConfiguration);
                }
            },
            None => return Err(Error::BadLightConfiguration),
        };

        let parsed_sensor_configs = match serde_json::to_value(&value.sensor_configs) {
            Ok(json) => json,
            Err(_) => {
                return Err(Error::BadSensorConfiguration);
            }
        };

        let result = ConfigModel {
            config_id,
            display_name: value.display_name,
            poll_period: parsed_poll_period,
            light_config: parsed_light_config,
            sensor_configs: parsed_sensor_configs,
        };

        Ok(result)
    }
}

impl TryFrom<ConfigModel> for ganymede::v2::Config {
    type Error = Error;

    fn try_from(value: ConfigModel) -> Result<ganymede::v2::Config> {
        let result = ganymede::v2::Config {
            uid: value.config_id.to_string(),
            display_name: value.display_name,
            poll_period: Some(prost_types::Duration {
                seconds: value.poll_period.num_seconds(),
                nanos: value.poll_period.subsec_nanos(),
            }),
            light_config: Some(
                match serde_json::from_value::<ganymede::v2::LightConfig>(value.light_config) {
                    Ok(config) => config,
                    Err(err) => {
                        log::error!("error parsing JSON from DB: {err}");
                        return Err(Error::GenericError(err.to_string()));
                    }
                },
            ),
            sensor_configs: match serde_json::from_value::<Vec<ganymede::v2::SensorConfig>>(value.sensor_configs) {
                Ok(configs) => configs,
                Err(err) => {
                    log::error!("error parsing JSON from DB: {err}");
                    return Err(Error::GenericError(err.to_string()));
                }
            },
        };

        Ok(result)
    }
}

#[cfg(test)]
mod tests {
    use uuid::uuid;

    use super::*;

    #[test]
    pub fn test_to_model() {
        let config = ganymede::v2::Config {
            uid: uuid::uuid!("00000000-0000-0000-1234-000000000000").to_string(),
            display_name: "this is a config".to_string(),
            poll_period: Some(prost_types::Duration {
                seconds: 1800,
                nanos: 0,
            }),
            light_config: Some(ganymede::v2::LightConfig {
                luminaires: [].to_vec(),
            }),
            sensor_configs: vec![ganymede::v2::SensorConfig {
                sensor: Some(ganymede::v2::sensor_config::Sensor::Am2320(
                    ganymede::v2::Am2320Config {
                        sda_port: 5,
                        scl_port: 6,
                    },
                )),
            }],
        };

        let model = ConfigModel::try_from(config).unwrap();
        assert_eq!(model.config_id, uuid::uuid!("00000000-0000-0000-1234-000000000000"));
        assert_eq!(model.display_name, "this is a config");
        assert_eq!(model.poll_period, chrono::TimeDelta::seconds(1800));
        assert_eq!(model.light_config, serde_json::json!({"luminaires": []}));
        assert_eq!(
            model.sensor_configs,
            serde_json::json!([{"sensor": {"am2320": {"sclPort": 6, "sdaPort": 5}}}])
        );
    }

    #[test]
    pub fn test_refuses_invalid_config_uid() {
        let config = ganymede::v2::Config {
            uid: "not-a-uuid".to_string(),
            display_name: "".to_string(),
            poll_period: Some(prost_types::Duration {
                seconds: 1800,
                nanos: 0,
            }),
            light_config: Some(ganymede::v2::LightConfig {
                luminaires: [].to_vec(),
            }),
            sensor_configs: Vec::new(),
        };

        let error = ConfigModel::try_from(config).unwrap_err();
        assert_eq!(error, Error::BadUuid);
    }

    #[test]
    pub fn test_refuses_invalid_light_config() {
        let config = ganymede::v2::Config {
            uid: uuid::uuid!("00000000-0000-0000-0000-000000000000").to_string(),
            display_name: "".to_string(),
            poll_period: Some(prost_types::Duration {
                seconds: 1800,
                nanos: 0,
            }),
            light_config: None,
            sensor_configs: Vec::new(),
        };

        let error = ConfigModel::try_from(config).unwrap_err();
        assert_eq!(error, Error::BadLightConfiguration);
    }

    #[test]
    pub fn test_refuses_poll_period_below_10_minutes() {
        let config = ganymede::v2::Config {
            uid: uuid::uuid!("00000000-0000-0000-0000-000000000000").to_string(),
            display_name: "".to_string(),
            poll_period: Some(prost_types::Duration { seconds: 360, nanos: 0 }),
            light_config: None,
            sensor_configs: Vec::new(),
        };

        let error = ConfigModel::try_from(config).unwrap_err();
        assert_eq!(error, Error::BadPollPeriod);
    }

    #[test]
    pub fn test_to_proto() {
        let config = ConfigModel {
            config_id: uuid!("00000000-0000-0000-0000-000000000001"),
            display_name: "This is a config!?".to_string(),
            poll_period: chrono::TimeDelta::seconds(1800),
            light_config: serde_json::json!({
                "luminaires": [
                    {
                        "port": 1,
                        "activeHigh": true,
                        "photoPeriod": [
                            {
                                "start": { "hour": 1, "minute": 2, "second": 3},
                                "stop": { "hour": 4, "minute": 5, "second": 6},
                                "intensity": 255
                            }
                        ]
                    }
                ]
            }),
            sensor_configs: serde_json::json!(
                [
                    {"sensor": {"am2320": {"sclPort": 6, "sdaPort": 5}}},
                    {"sensor": {"am2320": {"sclPort": 1, "sdaPort": 2}}}
                ]
            ),
        };

        let result = ganymede::v2::Config::try_from(config).unwrap();
        assert_eq!(result.uid, "00000000-0000-0000-0000-000000000001");
        assert_eq!(result.display_name, "This is a config!?");
        assert_eq!(
            result.poll_period,
            Some(prost_types::Duration {
                seconds: 1800,
                nanos: 0
            })
        );
        assert_eq!(
            result.light_config,
            Some(ganymede::v2::LightConfig {
                luminaires: [ganymede::v2::Luminaire {
                    port: 1,
                    active_high: true,
                    photo_period: [ganymede::v2::luminaire::DailySchedule {
                        start: Some(ganymede::v2::Time {
                            hour: 1,
                            minute: 2,
                            second: 3
                        }),
                        stop: Some(ganymede::v2::Time {
                            hour: 4,
                            minute: 5,
                            second: 6
                        }),
                        intensity: 255
                    }]
                    .to_vec()
                }]
                .to_vec()
            })
        );
        assert_eq!(
            result.sensor_configs,
            [
                ganymede::v2::SensorConfig {
                    sensor: Some(ganymede::v2::sensor_config::Sensor::Am2320(
                        ganymede::v2::Am2320Config {
                            sda_port: 5,
                            scl_port: 6
                        }
                    ))
                },
                ganymede::v2::SensorConfig {
                    sensor: Some(ganymede::v2::sensor_config::Sensor::Am2320(
                        ganymede::v2::Am2320Config {
                            sda_port: 2,
                            scl_port: 1
                        }
                    ))
                },
            ]
            .to_vec()
        );
    }
}
