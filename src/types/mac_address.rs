use crate::{Error, Result};

#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, sqlx::Type)]
#[sqlx(transparent)]
pub struct MacAddress(String);

impl MacAddress {
    pub fn try_parse(input: &str) -> Result<MacAddress> {
        if input.len() != 17 {
            return Err(Error::BadMacAddress);
        }

        for (i, c) in input.chars().enumerate() {
            if (i + 1) % 3 == 0 {
                if c != ':' {
                    return Err(Error::BadMacAddress);
                }
            } else if !c.is_ascii_hexdigit() {
                return Err(Error::BadMacAddress);
            }
        }

        Ok(MacAddress {
            0: input.to_ascii_lowercase(),
        })
    }

    pub fn to_string(&self) -> &String {
        &self.0
    }
}

impl TryFrom<String> for MacAddress {
    type Error = Error;

    fn try_from(value: String) -> Result<MacAddress> {
        MacAddress::try_parse(&value)
    }
}

impl TryFrom<&str> for MacAddress {
    type Error = Error;

    fn try_from(value: &str) -> Result<MacAddress> {
        MacAddress::try_parse(value)
    }
}

impl From<MacAddress> for String {
    fn from(value: MacAddress) -> String {
        value.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_try_into_mac() {
        assert!(MacAddress::try_from("00:00:00:00:00:00".to_string()).is_ok());
        assert!(MacAddress::try_from("AA:bb:CC:dd:EE:ff".to_string()).is_ok());

        assert!(MacAddress::try_from("not-a-mac".to_string()).is_err());
        assert!(MacAddress::try_from("aa:aa:aa:aa:aa:GG").is_err());
        assert!(MacAddress::try_from("aa:aa:aa:aa:aa:aa:aa").is_err());
        assert!(MacAddress::try_from("aaaaaaaaaaaa").is_err());
        assert!(MacAddress::try_from("").is_err());
    }
}
