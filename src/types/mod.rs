pub mod celsius;
mod fractional;
pub mod mac_address;

pub type Celsius = celsius::Celsius;
pub type MacAddress = mac_address::MacAddress;
pub type RelativeHumidity = fractional::Fractional;
