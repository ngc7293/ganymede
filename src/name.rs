use base64::{prelude::BASE64_URL_SAFE_NO_PAD as base64, Engine};

use crate::error::Error;

#[derive(Debug, PartialEq)]
pub struct ResourceName {
    fragments: Vec<(String, uuid::Uuid)>,
}

impl ResourceName {
    pub fn try_from(name: &str, format: &str) -> Result<ResourceName, Error> {
        let mut results = Vec::<(String, uuid::Uuid)>::new();

        if name.matches("/").count() != format.matches("/").count() {
            return Err(Error::NameError);
        }

        let mut last_key: Option<&str> = None;
        let iter = std::iter::zip(name.split("/"), format.split("/"));

        for tokens in iter.into_iter() {
            if tokens.1 == "{}" {
                if let Ok(binary) = base64.decode(tokens.0) {
                    if let Ok(uuid) = uuid::Uuid::try_from(binary) {
                        if let Some(key) = last_key.take() {
                            results.push((String::from(key), uuid));
                            continue;
                        }
                    }
                }

                return Err(Error::NameError);
            }

            if tokens.0 != tokens.1 {
                return Err(Error::NameError);
            }

            last_key = Some(tokens.0);
        }

        Ok(ResourceName { fragments: results })
    }

    pub fn new(fragments: Vec<(&'static str, uuid::Uuid)>) -> ResourceName {
        ResourceName {
            fragments: fragments.into_iter().map(|(k, v)| (k.into(), v)).collect(),
        }
    }

    pub fn get(&self, key: &'static str) -> Result<uuid::Uuid, Error> {
        for (k, v) in self.fragments.iter() {
            if k == key {
                return Ok(v.clone());
            }
        }

        Err(Error::NameError)
    }

    pub fn to_string(self) -> String {
        self.fragments
            .into_iter()
            .map(|(k, v)| format!("{}/{}", k, base64.encode(v)))
            .collect::<Vec<String>>()
            .join("/")
    }
}

impl From<ResourceName> for String {
    fn from(value: ResourceName) -> Self {
        value.to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_with_single_uuid() {
        assert_eq!(
            ResourceName::try_from("devices/QasxIsREQqivPuKUwY--OA", "devices/{}"),
            Ok(ResourceName {
                fragments: vec!(("devices".into(), uuid::uuid!("41ab3122-c444-42a8-af3e-e294c18fbe38")))
            })
        );
    }

    #[test]
    fn test_parse_with_many_uuid() {
        assert_eq!(
            ResourceName::try_from(
                "devicetypes/QasxIsREQqivPuKUwY--OA/features/dM9s94plQJKyQAcKAncw5Q",
                "devicetypes/{}/features/{}"
            ),
            Ok(ResourceName {
                fragments: vec!(
                    (
                        "devicetypes".into(),
                        uuid::uuid!("41ab3122-c444-42a8-af3e-e294c18fbe38")
                    ),
                    ("features".into(), uuid::uuid!("74cf6cf7-8a65-4092-b240-070a027730e5")),
                )
            })
        );
    }

    #[test]
    fn test_parse_with_mismatch_resource_name() {
        assert_eq!(
            ResourceName::try_from("devices/QasxIsREQqivPuKUwY--OA", "features/{}"),
            Err(Error::NameError)
        );
    }

    #[test]
    fn test_parse_with_invalid_uuid() {
        assert_eq!(
            ResourceName::try_from("features/not-a-uuid", "features/{}"),
            Err(Error::NameError)
        );
        assert_eq!(
            ResourceName::try_from("features//info", "features/{}/info"),
            Err(Error::NameError)
        );
    }

    #[test]
    fn test_parse_with_shorter_value() {
        assert_eq!(
            ResourceName::try_from("features/", "features/{}"),
            Err(Error::NameError)
        );
        assert_eq!(ResourceName::try_from("features", "features/{}"), Err(Error::NameError));
        assert_eq!(ResourceName::try_from("/", "features/{}"), Err(Error::NameError));
    }

    #[test]
    fn test_parse_with_empty_values() {
        assert_eq!(
            ResourceName::try_from("", ""),
            Ok(ResourceName { fragments: Vec::new() })
        );
    }

    #[test]
    fn test_stringify() {
        assert_eq!(
            ResourceName {
                fragments: vec!(
                    (
                        "devicetypes".into(),
                        uuid::uuid!("41ab3122-c444-42a8-af3e-e294c18fbe38")
                    ),
                    ("features".into(), uuid::uuid!("74cf6cf7-8a65-4092-b240-070a027730e5")),
                )
            }
            .to_string(),
            String::from("devicetypes/QasxIsREQqivPuKUwY--OA/features/dM9s94plQJKyQAcKAncw5Q"),
        );
    }

    #[test]
    fn test_stringify_with_empty_value() {
        assert_eq!(ResourceName { fragments: Vec::new() }.to_string(), String::new(),);
    }
}
