// automatically generated by the FlatBuffers compiler, do not modify

use crate::CommonTypes_generated::*;
use std::cmp::Ordering;
use std::mem;

extern crate flatbuffers;
use self::flatbuffers::EndianScalar;

#[allow(unused_imports, dead_code)]
pub mod tempest {

    use crate::CommonTypes_generated::*;
    use std::cmp::Ordering;
    use std::mem;

    extern crate flatbuffers;
    use self::flatbuffers::EndianScalar;
    #[allow(unused_imports, dead_code)]
    pub mod definition {

        use crate::CommonTypes_generated::*;
        use std::cmp::Ordering;
        use std::mem;

        extern crate flatbuffers;
        use self::flatbuffers::EndianScalar;

        // struct PrimitiveMeshData, aligned to 4
        #[repr(C, align(4))]
        #[derive(Clone, Copy, Debug, PartialEq)]
        pub struct PrimitiveMeshData {
            meshlets_offset_: u32,
            meshlets_count_: u32,
            material_index_: u32,
        } // pub struct PrimitiveMeshData
        impl flatbuffers::SafeSliceAccess for PrimitiveMeshData {}
        impl<'a> flatbuffers::Follow<'a> for PrimitiveMeshData {
            type Inner = &'a PrimitiveMeshData;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                <&'a PrimitiveMeshData>::follow(buf, loc)
            }
        }
        impl<'a> flatbuffers::Follow<'a> for &'a PrimitiveMeshData {
            type Inner = &'a PrimitiveMeshData;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                flatbuffers::follow_cast_ref::<PrimitiveMeshData>(buf, loc)
            }
        }
        impl<'b> flatbuffers::Push for PrimitiveMeshData {
            type Output = PrimitiveMeshData;
            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(
                        self as *const PrimitiveMeshData as *const u8,
                        Self::size(),
                    )
                };
                dst.copy_from_slice(src);
            }
        }
        impl<'b> flatbuffers::Push for &'b PrimitiveMeshData {
            type Output = PrimitiveMeshData;

            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(
                        *self as *const PrimitiveMeshData as *const u8,
                        Self::size(),
                    )
                };
                dst.copy_from_slice(src);
            }
        }

        impl PrimitiveMeshData {
            pub fn new<'a>(
                _meshlets_offset: u32,
                _meshlets_count: u32,
                _material_index: u32,
            ) -> Self {
                PrimitiveMeshData {
                    meshlets_offset_: _meshlets_offset.to_little_endian(),
                    meshlets_count_: _meshlets_count.to_little_endian(),
                    material_index_: _material_index.to_little_endian(),
                }
            }
            pub fn meshlets_offset<'a>(&'a self) -> u32 {
                self.meshlets_offset_.from_little_endian()
            }
            pub fn meshlets_count<'a>(&'a self) -> u32 {
                self.meshlets_count_.from_little_endian()
            }
            pub fn material_index<'a>(&'a self) -> u32 {
                self.material_index_.from_little_endian()
            }
        }

        // struct MeshData, aligned to 4
        #[repr(C, align(4))]
        #[derive(Clone, Copy, Debug, PartialEq)]
        pub struct MeshData {
            primitive_mesh_offset_: u32,
            primitive_mesh_count_: u32,
        } // pub struct MeshData
        impl flatbuffers::SafeSliceAccess for MeshData {}
        impl<'a> flatbuffers::Follow<'a> for MeshData {
            type Inner = &'a MeshData;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                <&'a MeshData>::follow(buf, loc)
            }
        }
        impl<'a> flatbuffers::Follow<'a> for &'a MeshData {
            type Inner = &'a MeshData;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                flatbuffers::follow_cast_ref::<MeshData>(buf, loc)
            }
        }
        impl<'b> flatbuffers::Push for MeshData {
            type Output = MeshData;
            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(self as *const MeshData as *const u8, Self::size())
                };
                dst.copy_from_slice(src);
            }
        }
        impl<'b> flatbuffers::Push for &'b MeshData {
            type Output = MeshData;

            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(
                        *self as *const MeshData as *const u8,
                        Self::size(),
                    )
                };
                dst.copy_from_slice(src);
            }
        }

        impl MeshData {
            pub fn new<'a>(_primitive_mesh_offset: u32, _primitive_mesh_count: u32) -> Self {
                MeshData {
                    primitive_mesh_offset_: _primitive_mesh_offset.to_little_endian(),
                    primitive_mesh_count_: _primitive_mesh_count.to_little_endian(),
                }
            }
            pub fn primitive_mesh_offset<'a>(&'a self) -> u32 {
                self.primitive_mesh_offset_.from_little_endian()
            }
            pub fn primitive_mesh_count<'a>(&'a self) -> u32 {
                self.primitive_mesh_count_.from_little_endian()
            }
        }

        // struct MeshMapping, aligned to 4
        #[repr(C, align(4))]
        #[derive(Clone, Copy, Debug, PartialEq)]
        pub struct MeshMapping {
            index_: u32,
            mesh_data_: MeshData,
        } // pub struct MeshMapping
        impl flatbuffers::SafeSliceAccess for MeshMapping {}
        impl<'a> flatbuffers::Follow<'a> for MeshMapping {
            type Inner = &'a MeshMapping;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                <&'a MeshMapping>::follow(buf, loc)
            }
        }
        impl<'a> flatbuffers::Follow<'a> for &'a MeshMapping {
            type Inner = &'a MeshMapping;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                flatbuffers::follow_cast_ref::<MeshMapping>(buf, loc)
            }
        }
        impl<'b> flatbuffers::Push for MeshMapping {
            type Output = MeshMapping;
            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(
                        self as *const MeshMapping as *const u8,
                        Self::size(),
                    )
                };
                dst.copy_from_slice(src);
            }
        }
        impl<'b> flatbuffers::Push for &'b MeshMapping {
            type Output = MeshMapping;

            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(
                        *self as *const MeshMapping as *const u8,
                        Self::size(),
                    )
                };
                dst.copy_from_slice(src);
            }
        }

        impl MeshMapping {
            pub fn new<'a>(_index: u32, _mesh_data: &'a MeshData) -> Self {
                MeshMapping {
                    index_: _index.to_little_endian(),
                    mesh_data_: *_mesh_data,
                }
            }
            pub fn index<'a>(&'a self) -> u32 {
                self.index_.from_little_endian()
            }
            #[inline]
            pub fn key_compare_less_than(&self, o: &MeshMapping) -> bool {
                self.index() < o.index()
            }

            #[inline]
            pub fn key_compare_with_value(&self, val: u32) -> ::std::cmp::Ordering {
                let key = self.index();
                key.cmp(&val)
            }
            pub fn mesh_data<'a>(&'a self) -> &'a MeshData {
                &self.mesh_data_
            }
        }

        // struct Meshlet, aligned to 4
        #[repr(C, align(4))]
        #[derive(Clone, Copy, Debug, PartialEq)]
        pub struct Meshlet {
            vertex_offset_: u32,
            vertex_count_: u32,
            triangle_offset_: u32,
            triangle_count_: u32,
        } // pub struct Meshlet
        impl flatbuffers::SafeSliceAccess for Meshlet {}
        impl<'a> flatbuffers::Follow<'a> for Meshlet {
            type Inner = &'a Meshlet;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                <&'a Meshlet>::follow(buf, loc)
            }
        }
        impl<'a> flatbuffers::Follow<'a> for &'a Meshlet {
            type Inner = &'a Meshlet;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                flatbuffers::follow_cast_ref::<Meshlet>(buf, loc)
            }
        }
        impl<'b> flatbuffers::Push for Meshlet {
            type Output = Meshlet;
            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(self as *const Meshlet as *const u8, Self::size())
                };
                dst.copy_from_slice(src);
            }
        }
        impl<'b> flatbuffers::Push for &'b Meshlet {
            type Output = Meshlet;

            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(*self as *const Meshlet as *const u8, Self::size())
                };
                dst.copy_from_slice(src);
            }
        }

        impl Meshlet {
            pub fn new<'a>(
                _vertex_offset: u32,
                _vertex_count: u32,
                _triangle_offset: u32,
                _triangle_count: u32,
            ) -> Self {
                Meshlet {
                    vertex_offset_: _vertex_offset.to_little_endian(),
                    vertex_count_: _vertex_count.to_little_endian(),
                    triangle_offset_: _triangle_offset.to_little_endian(),
                    triangle_count_: _triangle_count.to_little_endian(),
                }
            }
            pub fn vertex_offset<'a>(&'a self) -> u32 {
                self.vertex_offset_.from_little_endian()
            }
            pub fn vertex_count<'a>(&'a self) -> u32 {
                self.vertex_count_.from_little_endian()
            }
            pub fn triangle_offset<'a>(&'a self) -> u32 {
                self.triangle_offset_.from_little_endian()
            }
            pub fn triangle_count<'a>(&'a self) -> u32 {
                self.triangle_count_.from_little_endian()
            }
        }

        // struct Material, aligned to 4
        #[repr(C, align(4))]
        #[derive(Clone, Copy, Debug, PartialEq)]
        pub struct Material {
            albedo_color_factor_: super::super::common::tempest::Color,
            metallic_factor_: f32,
            roughness_factor_: f32,
            albedo_color_texture_index_: u32,
            metallic_roughness_texture_index_: u32,
        } // pub struct Material
        impl flatbuffers::SafeSliceAccess for Material {}
        impl<'a> flatbuffers::Follow<'a> for Material {
            type Inner = &'a Material;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                <&'a Material>::follow(buf, loc)
            }
        }
        impl<'a> flatbuffers::Follow<'a> for &'a Material {
            type Inner = &'a Material;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                flatbuffers::follow_cast_ref::<Material>(buf, loc)
            }
        }
        impl<'b> flatbuffers::Push for Material {
            type Output = Material;
            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(self as *const Material as *const u8, Self::size())
                };
                dst.copy_from_slice(src);
            }
        }
        impl<'b> flatbuffers::Push for &'b Material {
            type Output = Material;

            #[inline]
            fn push(&self, dst: &mut [u8], _rest: &[u8]) {
                let src = unsafe {
                    ::std::slice::from_raw_parts(
                        *self as *const Material as *const u8,
                        Self::size(),
                    )
                };
                dst.copy_from_slice(src);
            }
        }

        impl Material {
            pub fn new<'a>(
                _albedo_color_factor: &'a super::super::common::tempest::Color,
                _metallic_factor: f32,
                _roughness_factor: f32,
                _albedo_color_texture_index: u32,
                _metallic_roughness_texture_index: u32,
            ) -> Self {
                Material {
                    albedo_color_factor_: *_albedo_color_factor,
                    metallic_factor_: _metallic_factor.to_little_endian(),
                    roughness_factor_: _roughness_factor.to_little_endian(),
                    albedo_color_texture_index_: _albedo_color_texture_index.to_little_endian(),
                    metallic_roughness_texture_index_: _metallic_roughness_texture_index
                        .to_little_endian(),
                }
            }
            pub fn albedo_color_factor<'a>(&'a self) -> &'a super::super::common::tempest::Color {
                &self.albedo_color_factor_
            }
            pub fn metallic_factor<'a>(&'a self) -> f32 {
                self.metallic_factor_.from_little_endian()
            }
            pub fn roughness_factor<'a>(&'a self) -> f32 {
                self.roughness_factor_.from_little_endian()
            }
            pub fn albedo_color_texture_index<'a>(&'a self) -> u32 {
                self.albedo_color_texture_index_.from_little_endian()
            }
            pub fn metallic_roughness_texture_index<'a>(&'a self) -> u32 {
                self.metallic_roughness_texture_index_.from_little_endian()
            }
        }

        pub enum GeometryDatabaseOffset {}
        #[derive(Copy, Clone, Debug, PartialEq)]

        pub struct GeometryDatabase<'a> {
            pub _tab: flatbuffers::Table<'a>,
        }

        impl<'a> flatbuffers::Follow<'a> for GeometryDatabase<'a> {
            type Inner = GeometryDatabase<'a>;
            #[inline]
            fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
                Self {
                    _tab: flatbuffers::Table { buf: buf, loc: loc },
                }
            }
        }

        impl<'a> GeometryDatabase<'a> {
            #[inline]
            pub fn init_from_table(table: flatbuffers::Table<'a>) -> Self {
                GeometryDatabase { _tab: table }
            }
            #[allow(unused_mut)]
            pub fn create<'bldr: 'args, 'args: 'mut_bldr, 'mut_bldr>(
                _fbb: &'mut_bldr mut flatbuffers::FlatBufferBuilder<'bldr>,
                args: &'args GeometryDatabaseArgs<'args>,
            ) -> flatbuffers::WIPOffset<GeometryDatabase<'bldr>> {
                let mut builder = GeometryDatabaseBuilder::new(_fbb);
                if let Some(x) = args.mappings {
                    builder.add_mappings(x);
                }
                if let Some(x) = args.materials {
                    builder.add_materials(x);
                }
                if let Some(x) = args.primitive_meshes {
                    builder.add_primitive_meshes(x);
                }
                if let Some(x) = args.meshlet_buffer {
                    builder.add_meshlet_buffer(x);
                }
                if let Some(x) = args.meshlet_indices_buffer {
                    builder.add_meshlet_indices_buffer(x);
                }
                if let Some(x) = args.vertex_buffer {
                    builder.add_vertex_buffer(x);
                }
                builder.finish()
            }

            pub const VT_VERTEX_BUFFER: flatbuffers::VOffsetT = 4;
            pub const VT_MESHLET_INDICES_BUFFER: flatbuffers::VOffsetT = 6;
            pub const VT_MESHLET_BUFFER: flatbuffers::VOffsetT = 8;
            pub const VT_PRIMITIVE_MESHES: flatbuffers::VOffsetT = 10;
            pub const VT_MATERIALS: flatbuffers::VOffsetT = 12;
            pub const VT_MAPPINGS: flatbuffers::VOffsetT = 14;

            #[inline]
            pub fn vertex_buffer(&self) -> Option<&'a [u8]> {
                self._tab
                    .get::<flatbuffers::ForwardsUOffset<flatbuffers::Vector<'a, u8>>>(
                        GeometryDatabase::VT_VERTEX_BUFFER,
                        None,
                    )
                    .map(|v| v.safe_slice())
            }
            #[inline]
            pub fn meshlet_indices_buffer(&self) -> Option<&'a [u8]> {
                self._tab
                    .get::<flatbuffers::ForwardsUOffset<flatbuffers::Vector<'a, u8>>>(
                        GeometryDatabase::VT_MESHLET_INDICES_BUFFER,
                        None,
                    )
                    .map(|v| v.safe_slice())
            }
            #[inline]
            pub fn meshlet_buffer(&self) -> Option<&'a [Meshlet]> {
                self._tab
                    .get::<flatbuffers::ForwardsUOffset<flatbuffers::Vector<Meshlet>>>(
                        GeometryDatabase::VT_MESHLET_BUFFER,
                        None,
                    )
                    .map(|v| v.safe_slice())
            }
            #[inline]
            pub fn primitive_meshes(&self) -> Option<&'a [PrimitiveMeshData]> {
                self._tab
                    .get::<flatbuffers::ForwardsUOffset<flatbuffers::Vector<PrimitiveMeshData>>>(
                        GeometryDatabase::VT_PRIMITIVE_MESHES,
                        None,
                    )
                    .map(|v| v.safe_slice())
            }
            #[inline]
            pub fn materials(&self) -> Option<&'a [Material]> {
                self._tab
                    .get::<flatbuffers::ForwardsUOffset<flatbuffers::Vector<Material>>>(
                        GeometryDatabase::VT_MATERIALS,
                        None,
                    )
                    .map(|v| v.safe_slice())
            }
            #[inline]
            pub fn mappings(&self) -> Option<&'a [MeshMapping]> {
                self._tab
                    .get::<flatbuffers::ForwardsUOffset<flatbuffers::Vector<MeshMapping>>>(
                        GeometryDatabase::VT_MAPPINGS,
                        None,
                    )
                    .map(|v| v.safe_slice())
            }
        }

        pub struct GeometryDatabaseArgs<'a> {
            pub vertex_buffer: Option<flatbuffers::WIPOffset<flatbuffers::Vector<'a, u8>>>,
            pub meshlet_indices_buffer: Option<flatbuffers::WIPOffset<flatbuffers::Vector<'a, u8>>>,
            pub meshlet_buffer: Option<flatbuffers::WIPOffset<flatbuffers::Vector<'a, Meshlet>>>,
            pub primitive_meshes:
                Option<flatbuffers::WIPOffset<flatbuffers::Vector<'a, PrimitiveMeshData>>>,
            pub materials: Option<flatbuffers::WIPOffset<flatbuffers::Vector<'a, Material>>>,
            pub mappings: Option<flatbuffers::WIPOffset<flatbuffers::Vector<'a, MeshMapping>>>,
        }
        impl<'a> Default for GeometryDatabaseArgs<'a> {
            #[inline]
            fn default() -> Self {
                GeometryDatabaseArgs {
                    vertex_buffer: None,
                    meshlet_indices_buffer: None,
                    meshlet_buffer: None,
                    primitive_meshes: None,
                    materials: None,
                    mappings: None,
                }
            }
        }
        pub struct GeometryDatabaseBuilder<'a: 'b, 'b> {
            fbb_: &'b mut flatbuffers::FlatBufferBuilder<'a>,
            start_: flatbuffers::WIPOffset<flatbuffers::TableUnfinishedWIPOffset>,
        }
        impl<'a: 'b, 'b> GeometryDatabaseBuilder<'a, 'b> {
            #[inline]
            pub fn add_vertex_buffer(
                &mut self,
                vertex_buffer: flatbuffers::WIPOffset<flatbuffers::Vector<'b, u8>>,
            ) {
                self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(
                    GeometryDatabase::VT_VERTEX_BUFFER,
                    vertex_buffer,
                );
            }
            #[inline]
            pub fn add_meshlet_indices_buffer(
                &mut self,
                meshlet_indices_buffer: flatbuffers::WIPOffset<flatbuffers::Vector<'b, u8>>,
            ) {
                self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(
                    GeometryDatabase::VT_MESHLET_INDICES_BUFFER,
                    meshlet_indices_buffer,
                );
            }
            #[inline]
            pub fn add_meshlet_buffer(
                &mut self,
                meshlet_buffer: flatbuffers::WIPOffset<flatbuffers::Vector<'b, Meshlet>>,
            ) {
                self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(
                    GeometryDatabase::VT_MESHLET_BUFFER,
                    meshlet_buffer,
                );
            }
            #[inline]
            pub fn add_primitive_meshes(
                &mut self,
                primitive_meshes: flatbuffers::WIPOffset<
                    flatbuffers::Vector<'b, PrimitiveMeshData>,
                >,
            ) {
                self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(
                    GeometryDatabase::VT_PRIMITIVE_MESHES,
                    primitive_meshes,
                );
            }
            #[inline]
            pub fn add_materials(
                &mut self,
                materials: flatbuffers::WIPOffset<flatbuffers::Vector<'b, Material>>,
            ) {
                self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(
                    GeometryDatabase::VT_MATERIALS,
                    materials,
                );
            }
            #[inline]
            pub fn add_mappings(
                &mut self,
                mappings: flatbuffers::WIPOffset<flatbuffers::Vector<'b, MeshMapping>>,
            ) {
                self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(
                    GeometryDatabase::VT_MAPPINGS,
                    mappings,
                );
            }
            #[inline]
            pub fn new(
                _fbb: &'b mut flatbuffers::FlatBufferBuilder<'a>,
            ) -> GeometryDatabaseBuilder<'a, 'b> {
                let start = _fbb.start_table();
                GeometryDatabaseBuilder {
                    fbb_: _fbb,
                    start_: start,
                }
            }
            #[inline]
            pub fn finish(self) -> flatbuffers::WIPOffset<GeometryDatabase<'a>> {
                let o = self.fbb_.end_table(self.start_);
                flatbuffers::WIPOffset::new(o.value())
            }
        }

        #[inline]
        pub fn get_root_as_geometry_database<'a>(buf: &'a [u8]) -> GeometryDatabase<'a> {
            flatbuffers::get_root::<GeometryDatabase<'a>>(buf)
        }

        #[inline]
        pub fn get_size_prefixed_root_as_geometry_database<'a>(
            buf: &'a [u8],
        ) -> GeometryDatabase<'a> {
            flatbuffers::get_size_prefixed_root::<GeometryDatabase<'a>>(buf)
        }

        pub const GEOMETRY_DATABASE_IDENTIFIER: &'static str = "TGDB";

        #[inline]
        pub fn geometry_database_buffer_has_identifier(buf: &[u8]) -> bool {
            return flatbuffers::buffer_has_identifier(buf, GEOMETRY_DATABASE_IDENTIFIER, false);
        }

        #[inline]
        pub fn geometry_database_size_prefixed_buffer_has_identifier(buf: &[u8]) -> bool {
            return flatbuffers::buffer_has_identifier(buf, GEOMETRY_DATABASE_IDENTIFIER, true);
        }

        pub const GEOMETRY_DATABASE_EXTENSION: &'static str = "tgb";

        #[inline]
        pub fn finish_geometry_database_buffer<'a, 'b>(
            fbb: &'b mut flatbuffers::FlatBufferBuilder<'a>,
            root: flatbuffers::WIPOffset<GeometryDatabase<'a>>,
        ) {
            fbb.finish(root, Some(GEOMETRY_DATABASE_IDENTIFIER));
        }

        #[inline]
        pub fn finish_size_prefixed_geometry_database_buffer<'a, 'b>(
            fbb: &'b mut flatbuffers::FlatBufferBuilder<'a>,
            root: flatbuffers::WIPOffset<GeometryDatabase<'a>>,
        ) {
            fbb.finish_size_prefixed(root, Some(GEOMETRY_DATABASE_IDENTIFIER));
        }
    } // pub mod Definition
} // pub mod Tempest
