#![allow(unused_imports)]
#![allow(non_snake_case)]
pub mod GeometryDatabase_generated;
pub mod Level_generated;
pub mod ShaderLibrary_generated;

pub use GeometryDatabase_generated::tempest::definition::*;
pub use Level_generated::tempest::definition::*;
pub use ShaderLibrary_generated::tempest::definition::*;

pub trait CreateInBuilder<'fbb> {
    type Item;
    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item>;
}

impl<'fbb> CreateInBuilder<'fbb> for &str {
    type Item = &'fbb str;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        builder.create_string(self)
    }
}

impl<'fbb> CreateInBuilder<'fbb> for &[u8] {
    type Item = flatbuffers::Vector<'fbb, u8>;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        builder.create_vector(self)
    }
}

#[macro_use]
pub extern crate flatbuffer_derive;

pub extern crate flatbuffers;
