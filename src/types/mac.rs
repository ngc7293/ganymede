#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, sqlx::Type)]
#[sqlx(transparent)]
pub struct Mac(String);

#[derive(Debug)]
pub enum Error {
    InvalidMac,
}

impl Mac {
    pub fn try_parse(input: &str) -> Result<Mac, Error> {
        if input.len() != 17 {
            return Err(Error::InvalidMac);
        }

        for (i, c) in input.chars().enumerate() {
            if (i + 1) % 3 == 0 {
                if c != ':' {
                    return Err(Error::InvalidMac);
                }
            } else if !c.is_ascii_hexdigit() {
                return Err(Error::InvalidMac);
            }
        }

        Ok(Mac {
            0: input.to_ascii_lowercase(),
        })
    }

    pub fn to_string(&self) -> &String {
        &self.0
    }
}

impl TryFrom<String> for Mac {
    type Error = Error;

    fn try_from(value: String) -> Result<Mac, Error> {
        Mac::try_parse(&value)
    }
}

impl TryFrom<&str> for Mac {
    type Error = Error;

    fn try_from(value: &str) -> Result<Mac, Error> {
        Mac::try_parse(value)
    }
}

impl From<Mac> for String {
    fn from(value: Mac) -> String {
        value.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_try_into_mac() {
        assert!(Mac::try_from("00:00:00:00:00:00".to_string()).is_ok());
        assert!(Mac::try_from("AA:bb:CC:dd:EE:ff".to_string()).is_ok());

        assert!(Mac::try_from("not-a-mac".to_string()).is_err());
        assert!(Mac::try_from("aa:aa:aa:aa:aa:GG").is_err());
        assert!(Mac::try_from("aa:aa:aa:aa:aa:aa:aa").is_err());
        assert!(Mac::try_from("aaaaaaaaaaaa").is_err());
        assert!(Mac::try_from("").is_err());
    }
}
