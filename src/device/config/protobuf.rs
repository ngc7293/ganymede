use crate::ganymede;

use super::{errors::ConfigError, model::ConfigModel};

impl TryFrom<ganymede::v2::Config> for ConfigModel {
    type Error = ConfigError;

    fn try_from(value: ganymede::v2::Config) -> Result<ConfigModel, ConfigError> {
        let config_id = match value.uid.as_str() {
            "" => uuid::Uuid::nil(),
            _ =>  uuid::Uuid::try_parse(&value.uid).map_err(|_| ConfigError::InvalidConfigId)?
        };

        let result = ConfigModel {
            config_id,
            domain_id: uuid::Uuid::nil(),
            display_name: value.display_name,
            light_config: match value.light_config {
                Some(light_config) => match serde_json::to_value(&light_config) {
                    Ok(json) => json,
                    Err(_) => {
                        return Err(ConfigError::InvalidLightConfig);
                    }
                },
                None => return Err(ConfigError::InvalidLightConfig),
            },
        };

        Ok(result)
    }
}

impl TryFrom<ConfigModel> for ganymede::v2::Config {
    type Error = ConfigError;

    fn try_from(value: ConfigModel) -> Result<ganymede::v2::Config, ConfigError> {
        let result = ganymede::v2::Config {
            uid: value.config_id.to_string(),
            display_name: value.display_name,
            light_config: Some(
                match serde_json::from_value::<ganymede::v2::LightConfig>(value.light_config) {
                    Ok(config) => config,
                    Err(err) => {
                        log::error!("error parsing JSON from DB: {err}");
                        return Err(ConfigError::InvalidLightConfig);
                    }
                },
            ),
        };

        Ok(result)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    pub fn test_to_model() {
        let config = ganymede::v2::Config {
            uid: uuid::uuid!("00000000-0000-0000-1234-000000000000").to_string(),
            display_name: "this is a config".to_string(),
            light_config: Some(ganymede::v2::LightConfig {
                luminaires: [].to_vec(),
            }),
        };

        let model = ConfigModel::try_from(config).unwrap();
        assert_eq!(
            model.config_id,
            uuid::uuid!("00000000-0000-0000-1234-000000000000")
        );
        assert_eq!(model.domain_id, uuid::Uuid::nil());
        assert_eq!(model.display_name, "this is a config");
        assert_eq!(model.light_config, serde_json::json!({"luminaires": []}));
    }

    #[test]
    pub fn test_refuses_invalid_config_uid() {
        let config = ganymede::v2::Config {
            uid: "not-a-uuid".to_string(),
            display_name: "".to_string(),
            light_config: Some(ganymede::v2::LightConfig {
                luminaires: [].to_vec(),
            }),
        };

        let error = ConfigModel::try_from(config).unwrap_err();
        assert_eq!(error, ConfigError::InvalidConfigId);
    }

    #[test]
    pub fn test_refuses_invalid_light_config() {
        let config = ganymede::v2::Config {
            uid: uuid::uuid!("00000000-0000-0000-0000-000000000000").to_string(),
            display_name: "".to_string(),
            light_config: None,
        };

        let error = ConfigModel::try_from(config).unwrap_err();
        assert_eq!(error, ConfigError::InvalidLightConfig);
    }

    #[test]
    pub fn test_to_proto() {
        let config = ConfigModel {
            config_id: uuid::uuid!("00000000-0000-0000-0000-000000000001"),
            domain_id: uuid::uuid!("00000000-0000-0000-0000-000000000002"),
            display_name: "This is a config!?".to_string(),
            light_config: serde_json::json!({
                "luminaires": [
                    {
                        "port": 1,
                        "usePwm": true,
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
        };

        let result = ganymede::v2::Config::try_from(config).unwrap();
        assert_eq!(result.uid, "00000000-0000-0000-0000-000000000001");
        assert_eq!(result.display_name, "This is a config!?");
        assert_eq!(
            result.light_config,
            Some(ganymede::v2::LightConfig {
                luminaires: [ganymede::v2::Luminaire {
                    port: 1,
                    use_pwm: true,
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
    }
}
