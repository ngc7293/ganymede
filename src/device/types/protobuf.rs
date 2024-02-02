use crate::error::Error;

pub trait TryFromProtobuf: Sized {
    type Input;

    fn try_from_protobuf(input: Self::Input, domain_id: uuid::Uuid, strip_names: bool) -> Result<Self, Error>;
}

pub trait ToProtobuf: Sized {
    type Output;

    fn to_protobuf(self, include_nested: bool) -> Self::Output;
}