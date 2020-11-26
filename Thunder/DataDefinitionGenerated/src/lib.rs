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

// Special case for Vec & slice of u8
impl<'fbb> CreateInBuilder<'fbb> for &[u8] {
    type Item = flatbuffers::Vector<'fbb, u8>;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        builder.create_vector_direct(self)
    }
}

impl<'fbb> CreateInBuilder<'fbb> for Vec<u8> {
    type Item = flatbuffers::Vector<'fbb, u8>;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        builder.create_vector_direct(self.as_slice())
    }
}

// Generic Vec<T> & [T] where T is another CreateInBuilder struct
// TODO: Possible this can be just a single implementation somehow
impl<'fbb, T> CreateInBuilder<'fbb> for &[T]
where
    T: CreateInBuilder<'fbb>,
    T: 'fbb,
{
    type Item = flatbuffers::Vector<'fbb, flatbuffers::ForwardsUOffset<T::Item>>;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        let offsets = self
            .iter()
            .map(|value| value.create_in_builder(builder))
            .collect::<Vec<flatbuffers::WIPOffset<T::Item>>>();
        builder.create_vector(offsets.as_slice())
    }
}

impl<'fbb, T> CreateInBuilder<'fbb> for Vec<T>
where
    T: CreateInBuilder<'fbb>,
    T: 'fbb,
{
    type Item = flatbuffers::Vector<'fbb, flatbuffers::ForwardsUOffset<T::Item>>;

    fn create_in_builder(
        &self,
        builder: &mut flatbuffers::FlatBufferBuilder<'fbb>,
    ) -> flatbuffers::WIPOffset<Self::Item> {
        let offsets = self
            .iter()
            .map(|value| value.create_in_builder(builder))
            .collect::<Vec<flatbuffers::WIPOffset<T::Item>>>();
        builder.create_vector(offsets.as_slice())
    }
}

#[macro_use]
pub extern crate flatbuffer_derive;

pub extern crate flatbuffers;
