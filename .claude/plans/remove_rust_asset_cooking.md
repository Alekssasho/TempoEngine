# Plan: Remove Old Rust Asset Cooking Code

All asset cooking has been moved from the Rust `Thunder` binary to the C++ `Maelstrom` tool.
This plan removes all Rust code that was exclusively part of the old cooking pipeline.

**What stays:** `Thunder/ShaderCompiler/`, `Thunder/code_generator/`, `Thunder/DataDefinitionGenerated/` (partial), `Thunder/flatbuffer-derive/`
**What goes:** everything else in Thunder except those four crates, plus all 3 git submodules

---

## Step 1 — Delete cooking source directories

Delete the following directories entirely:

- `Thunder/src/` — main cooking binary (LevelResource, MeshResource, TextureResource, GeometryDatabaseResource, TextureDatabaseResource, AudioDatabaseResource, EntitiesWorldResource, PhysicsWorldResource)
- `Thunder/Cargo.toml` — manifest for the main Thunder binary (the crate whose source is Thunder/src/)
- `Thunder/physics-handler/` — PhysX wrapper for cooking collision meshes; PhysX is gone engine-wide, Maelstrom cooks Jolt physics directly
- `Thunder/flecs-rs/` — Rust ECS bindings, only used by EntitiesWorldResource during cooking
- `Thunder/flecs-rs-derive/` — proc macros for flecs-rs, no other consumer
- `Thunder/Components/` — C++ bindgen FFI bindings, only used by flecs-rs
- `Thunder/math/` — glam wrapper + TRS struct for GLTF coordinate conversion, only used by Thunder cooking and geometry_database_explorer
- `Thunder/geometry_database_explorer/` — GUI viewer for geometry databases, depends on math and mesh_shader which are cooking infrastructure
- `Thunder/shaders/` — contains mesh_shader (SPIR-V shader used only by geometry_database_explorer)

---

## Step 2 — Remove the 3 git submodules

Run the following commands from the repo root:

```bash
git submodule deinit -f ThirdParty/gltf
git rm ThirdParty/gltf
rm -rf .git/modules/ThirdParty/gltf

git submodule deinit -f ThirdParty/meshopt-rs
git rm ThirdParty/meshopt-rs
rm -rf .git/modules/ThirdParty/meshopt-rs

git submodule deinit -f ThirdParty/Compressonator
git rm ThirdParty/Compressonator
rm -rf .git/modules/ThirdParty/Compressonator
```

Rationale:
- `ThirdParty/gltf` — custom gltf-rs fork, used by MeshResource for GLTF loading. Maelstrom uses cgltf (vcpkg).
- `ThirdParty/meshopt-rs` — custom meshopt Rust bindings, used by MeshResource for vertex optimization. Maelstrom uses meshoptimizer (vcpkg).
- `ThirdParty/Compressonator` — GPU texture compression, used by TextureResource for BC7/BC1. Maelstrom uses NVTT (vcpkg).

---

## Step 3 — Clean up Cargo.toml and DataDefinitionGenerated

### 3a. Rewrite root `Cargo.toml`

Replace the current workspace members and patch section with:

```toml
[workspace]
members = [
    "Thunder/ShaderCompiler",
    "Thunder/code_generator",
]
```

Remove entirely:
- All commented-out members for deleted crates (`Thunder`, `Thunder/DataDefinitionGenerated`, `Thunder/Components`, `Thunder/flecs-rs`, `Thunder/flecs-rs-derive`, `Thunder/flatbuffer-derive`, `Thunder/physics-handler`, `Thunder/math`, `Thunder/geometry_database_explorer`, `Thunder/shaders/mesh_shader`)
- The entire `[patch.crates-io]` section (gltf, meshopt, and spirv-* patches are all gone with the submodules and geometry_database_explorer)

Note: `Thunder/DataDefinitionGenerated` and `Thunder/flatbuffer-derive` are kept on disk (used by ShaderCompiler as path dependencies) but do not need to be workspace members.

### 3b. Trim Thunder/DataDefinitionGenerated

`ShaderCompiler` depends on `DataDefinitionGenerated` only for `ShaderLibrary` types. The following generated files are cooking-only and should be deleted:

- `Thunder/DataDefinitionGenerated/src/audio_database_generated.rs` (or AudioDatabase_generated.rs — check exact filename)
- `Thunder/DataDefinitionGenerated/src/geometry_database_generated.rs`
- `Thunder/DataDefinitionGenerated/src/texture_database_generated.rs`
- `Thunder/DataDefinitionGenerated/src/level_generated.rs`

Then remove the corresponding `mod` declarations from `Thunder/DataDefinitionGenerated/src/lib.rs`.

Keep:
- `shader_library_generated.rs`
- `common_types_generated.rs`

### 3c. Verify the build

Run from the repo root:

```bash
cargo build
```

Both `ShaderCompiler` and `code_generator` must compile without errors. Fix any remaining references if compilation fails.
