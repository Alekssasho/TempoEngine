#![allow(unused_imports)]
#![allow(non_snake_case)]

pub mod AudioDatabase_generated;
pub mod CommonTypes_generated;
pub mod GeometryDatabase_generated;
pub mod Level_generated;
pub mod ShaderLibrary_generated;

// Common types have different namespace, due to bug in flatbuffer
// where Rust code which share namespace is omitted
// but this is not the way rust works and it breaks.
// Keep tabs on https://github.com/google/flatbuffers/issues/5589 which
// should track the issue
pub use AudioDatabase_generated::tempest::definition::*;
pub use CommonTypes_generated::common::tempest::*;
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

// String and &str impl
impl<'fbb> CreateInBuilder<'fbb> for &str {
    type Item = &'fbb str;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        builder.create_string(self)
    }
}

impl<'fbb> CreateInBuilder<'fbb> for String {
    type Item = &'fbb str;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        builder.create_string(&self)
    }
}

#[macro_use]
pub extern crate flatbuffer_derive;

pub extern crate flatbuffers;
