# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TempoEngine is a custom 3D game engine for Windows targeting a minimal, data-oriented runtime. Core philosophy: runtime assets are binary, memcpy-able files (no random access); all tooling (asset cooking, code generation) is written in Rust.

## Build System

**Project generation via Sharpmake** (C# DSL that generates `.vcxproj`/`.sln` files):
- Source: `sharpmake/*.sharpmake.cs`
- Generated solution: `TempoEngine.sln` at root (also `sharpmake/sharpmake_debugsolution.vs2022.sln`)
- After modifying any `.sharpmake.cs` file, regenerate the solution before building in VS

**Rust tooling** (asset cooker / code generator):
```
cargo build --release          # build all Rust tools
cargo build -p code_generator  # build just the code generator
```

**C++ build**: Open `TempoEngine.sln` in Visual Studio 2022. There are two configurations: Debug and Release, targeting win64 only.

**C++ compiler settings** (defined in `sharpmake/common.sharpmake.cs`):
- C++20, no exceptions (`/EHs-c-`), no RTTI
- Warnings as errors (Level 3)
- `NOMINMAX`, `WIN32_LEAN_AND_MEAN`, `_HAS_EXCEPTIONS=0`

## Three-Tier Architecture

```
Thunder/          ← Rust tooling (code gen, shader compiler, asset cooking)
Tempest/          ← C++ engine library (linked by executables)
Spark/ Maelstrom/ ← C++ executables built on Tempest
```

### Thunder (Rust)
- `Thunder/code_generator/` — reads `DataSchemas/Components.txt`, runs Jinja2 templates, emits `Tempest/Generated/Components.h/.cpp`
- `Thunder/ShaderCompiler/` — HLSL → compiled shaders
- `Thunder/src/` — main asset cooker (GLTF → `.tlb` binary level format via FlatBuffers)
- The code generator runs automatically as a VS pre-build step before C++ compilation

### Tempest (C++ library)
Key subsystems under `Tempest/`:
- **Graphics/** — DirectX 12 renderer; `Dx12/` is the backend, `Features/` holds render passes, `Managers/` holds texture/mesh managers
- **World/** — flecs ECS integration; `Components/` definitions, `GameplayFeatures/` game systems, `TaskGraph/` scheduling
- **Physics/** — Jolt Physics integration
- **Job/** — fiber-based job system (fiber-safe threading enabled globally)
- **Animation/** — skeletal animation playback
- **Audio/** — 3D spatial audio
- **Resources/** — runtime asset loading
- **Generated/** — auto-generated from `DataSchemas/Components.txt`; **do not edit by hand**

### Applications
- **Spark** (`Spark/Main.cpp`) — WinMain game runtime
- **Maelstrom** (`Maelstrom/Main.cpp`) — level compiler, converts GLTF scenes to binary `.tlb` levels

## Component Definition System

Components are declared in `DataSchemas/Components.txt` using a custom DSL:
```
[Inherit]               ← component data is inherited by child entities in flecs
[NoReflection]          ← skip reflection/serialization codegen
[StorageType(uint32_t)] ← field stored as a different underlying type
Component Foo { Bar : SomeType; }
Tag SomeTag;
```
After editing `Components.txt`, rebuild the `code_generator` Rust crate and run the custom build step (or let VS trigger it) to regenerate `Tempest/Generated/Components.h/.cpp`.

## Dependencies

**C++ via vcpkg** (see `vcpkg.json`): flecs, Jolt Physics, glm, DirectX 12, ImGui (dx12), Tracy profiler, EASTL, FlatBuffers, cgltf, stb, NVTT, Meshoptimizer, Gainput.

Custom overlay ports live in `OverlayPorts/` (flecs, joltphysics).

**Rust** (see `Cargo.toml` workspace): tokio, flecs-rs (custom bindings), gltf, meshopt, basis-universal, tera (templating), nom (parsing).

Git submodules: `ThirdParty/gltf`, `ThirdParty/meshopt-rs`, `ThirdParty/Compressonator`.

## Asset Pipeline

1. Author scene in GLTF format
2. Run Maelstrom to cook it: produces a `.tlb` binary (FlatBuffers) in `Maelstrom/Levels/`
3. Run Spark which loads `.tlb` levels directly at runtime — no runtime GLTF parsing

Compiled levels: `Level_village.tlb`, `Level_car3.tlb`, `Level_CastleFight.tlb`, `Level_AnimationExplorer.tlb`

## Binaries Output

- `Binaries/Debug/` and `Binaries/Release/` — all executables and DLLs (including Tracy DLL copies)
- `Intermediate/` — MSVC intermediate files
- `target/` — Rust build artifacts
- `projects/` — generated VS project files (from Sharpmake)
