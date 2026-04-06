---
name: TempoEngine Project Overview
description: High-level architecture, build system, and tech stack for TempoEngine
type: project
---

Custom 3D game engine: C++ runtime (Tempest library) + Rust tooling (Thunder). Windows 64-bit only, Visual Studio 2022, C++20, DirectX 12.

**Why:** Game engine built from scratch with minimal, data-oriented runtime using binary assets; no runtime GLTF/JSON parsing.

**How to apply:** When suggesting changes, respect no-exceptions/no-RTTI constraints. Code generation from DataSchemas/Components.txt is auto-run by VS pre-build step — don't edit Generated/ files directly. Sharpmake generates the .sln; edit .sharpmake.cs files to change project structure.

Key tech: flecs (ECS), Jolt Physics, Tracy profiler, EASTL, FlatBuffers, ImGui (DX12), vcpkg for C++ deps, cargo for Rust deps.
