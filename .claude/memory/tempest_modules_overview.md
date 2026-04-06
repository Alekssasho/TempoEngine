---
name: Tempest C++ Modules Detailed Overview
description: Deep implementation details for every Tempest subsystem — engine core, graphics, ECS, physics, audio, animation, job system, platform, resources
type: project
---

Deep dive into Tempest C++ source code. Use this when working on any engine subsystem.

**Why:** Explored all source files in Tempest/ in depth to document architecture, patterns, and in-progress work.

**How to apply:** Reference specific sections before suggesting changes to any subsystem. Check "Known Issues / TODOs" when planning new work.

---

## Engine Core (Engine.h, Game.cpp)

Engine class owns all subsystems. Main loop is a chain of fiber jobs, not a while loop:
1. `InitializeWindowJob` (Windows thread) — window setup, then spawns load job
2. Load job (worker) — async-parallel loads for all databases via counters, then `World::LoadFromLevel()` + `Renderer::InitializeAfterLevelLoad()`
3. `DoFrameJob` (repeating) — input → message pump (Windows thread) → ImGui → `World::Update()` → `Audio::Update()` → `Renderer::GatherWorldData()` → `Renderer::RenderFrame()`

**TODO:** delta time is hardcoded at 1/60s.

---

## World / ECS (World/, Generated/Components.h)

Built on **flecs**. `World::LoadFromLevel()` deserializes entities from binary `.tlb` level, wires job system for multi-threaded system execution, enables flecs REST API for live debugging.

All components defined in `DataSchemas/Components.txt`, code-generated into `Tempest/Generated/Components.h/.cpp`.

Component inventory:
- Transform (quat rotation, vec3 position/scale)
- StaticMesh / SkeletonMesh (MeshHandle, BoneTransforms)
- AnimationInfo / AnimationController (indices, state, time)
- PhysicsBody (Jolt BodyID)
- LightColorInfo (color, intensity)
- CameraController / VehicleController (input map index)
- Faction, Factory, NavigationData
- Movement (velocity), MovementInfo (speed), LaneMovement (lane iterator)
- Presence (radius), Health, AttackInfo, Attacking, SoundSource
- Tags: DirectionalLight, SimpleMovement, Attacks, SoundListener, Castle, Boids

GameplayFeatures — each registers flecs systems/observers in PrepareSystems():

| Feature | Systems |
|---|---|
| PhysicsFeature | Body add/remove observers, kinematic movement (PreUpdate), Transform←Jolt (PostUpdate) |
| InputControllerFeature | WASD camera, mouse look; vehicle input WIP/commented out |
| AnimationController | Observer inits bone transforms; system drives animation by time |
| SoldierMovementFeature | Lane pathfinding, velocity→movement, idle/walk animation transitions |
| BattleFeature | Target acquisition (awareness radius + faction), attack cooldown, damage, sound |
| FactorySpawner | Timed spawning, faction inheritance, relationship assignment |
| SpaceLocation | 10×10 spatial grid rebuilt each frame; used by BattleFeature for O(1) proximity |
| SoundFeature | Queries SoundSource+SoundListener, 3D attenuation + panning → AudioManager |
| HealthManagement | Destroys entities when health ≤ 0 |

`Tasks.h` has an older task graph system (ParallelMultiMap, ExecuteFunction) largely superseded by flecs systems.

---

## Graphics (Graphics/)

Three-layer design: **RenderFeature** (game-side data) → **RenderGraph** (frame resource scheduling) → **Dx12Backend** (API calls).

### Renderer
- Loads GeometryDatabase + TextureDatabase async on startup
- Registers meshlets, materials, textures into descriptor heap
- Per frame: GatherWorldData() → each RenderFeature populates FrameData → RenderFrame() builds and compiles render graph

### RenderGraph
Frame graph pattern. Two passes currently:
1. Shadow pass — 2048×2048 D32_FLOAT shadow map
2. Main pass — renders to backbuffer, reads shadow map

RenderGraphBuilder declares resource reads/writes; Compile() produces a RendererCommandList. Barriers derived automatically.

### Render Features
- StaticMeshFeature — Transform+StaticMesh → FrameData mesh transforms
- SkeletonMeshFeature — adds BoneTransforms → double-buffered ExtraDataBuffer
- LightsFeature — data only; gathers directional lights
- DebugFeature — draws debug rects/cubes

### Dx12Backend
Interprets RendererCommandList, issues D3D12 calls. Managers:
- PipelineManager — creates/caches ID3D12PipelineState
- BufferManager — VB/IB/CB allocation, CPU-mapped writes
- TextureManager — texture resource creation
- ConstantBufferDataManager — 256-byte aligned ring buffer for per-frame constants
- TemporaryTextureManager — per-frame scratch textures with state tracking

**Double buffering:** ExtraDataBuffer alternates even/odd frames so CPU writes bone matrices while GPU reads previous frame.

ShaderResourceSlot enum maps descriptor heap slots: Meshlets, MeshletIndices, MeshletVertices, MeshletSkeletonVertices, Materials, ExtraDataEvenFrame, ExtraDataOddFrame, TextureStart (dynamic range).

---

## Job System (Job/)

Fiber-based. Workers pick jobs from a lock-free queue; each job runs in a fiber. Waiting on a Counter yields the fiber back to the pool — the thread picks up the next ready fiber instead of blocking. Thread affinity tags (Windows, Worker) ensure PumpMessages() always runs on the OS window thread. Tracy integration gives per-fiber profiling.

---

## Physics (Physics/)

Wraps **Jolt Physics** behind PhysicsManager. Pimpl hides all Jolt types from headers.

Custom JoltJobSystem adapter routes Jolt's internal parallel jobs through Tempest's fiber job system — barrier implementation partially commented out (WIP).

Runtime flow:
- LoadDatabase() reads PhysicsDatabase.fbs (cooked by Maelstrom)
- PatchWorldComponents() adds PhysicsBody to entities after level load
- PreUpdate: SetVelocity() for kinematic bodies (input-driven movement)
- PostUpdate: CopyTransformFromBody() reads Jolt results → ECS Transform
- Update() advances simulation one step

**MirrorToPhysicsBodies (ECS Transform → Jolt) is currently commented out** — only the readback direction is active. Needed for teleport / scripted placement.

**Vehicle physics (VehicleController component exists) has no Jolt implementation yet** — old PhysX MirrorFromPhysicsCar code was removed, nothing replaced it.

---

## Animation (Animation/)

Offline-resampled skeletal animation in AnimationDatabase.fbs. Data: skeletons → bones (parent indices) → per-bone frames at 30 FPS.

Key methods:
- InitializeBoneTransforms() — rest pose via hierarchical parent × local transform
- ApplyFrameFromAnimation() — writes one frame's bone matrices
- ApplyAnimationWithTime() — selects frame by currentTime / frameTime
- ApplyInverseBindMatrices() — final skinning step

**No blending, no root motion. Skeleton index hardcoded to 0 in AnimationController.**

---

## Audio (Audio/)

WASAPI for output, STB Vorbis for Ogg streaming, custom Freeverb for reverb.

Lock-free AudioSamplesRingBuffer between game thread and WASAPI callback. AudioManager::Update() mixes active sound effects (distance attenuation + stereo panning) and pushes samples into ring buffer. WASAPI callback drains it on audio thread.

Reverb chain: 4 parallel comb filters (feedback + damping) → 2 all-pass filters for diffusion.
AVX2 SIMD used for mixing loop.

---

## Platform / Input (Platform/)

WindowsPlatform: window creation, PumpMessages() (must run on Windows thread), title updates.
InputManager wraps **Gainput** with named input maps (CameraMovement, VehicleMovement). Vehicle maps exist but consuming feature code is commented out.

---

## Resource System (Resources/)

Simple cache: ResourceLoader::Load<T>() reads FlatBuffers file, verifies in debug, caches in memory (no unloading). All databases share same path: GeometryDatabase, TextureDatabase, AnimationDatabase, PhysicsDatabase, SoundDatabase, Level.

---

## Per-Frame Data Flow

```
World::Update()             ← flecs runs all systems in parallel via job system
  PhysicsFeature            ← PreUpdate: SetVelocity on kinematic bodies
  Input/Movement/Battle     ← game logic
  PhysicsFeature            ← PostUpdate: Jolt → Transform
  AnimationController       ← update bone matrices
Audio::Update()             ← mix sounds into ring buffer
Renderer::GatherWorldData() ← RenderFeatures read ECS into FrameData
Renderer::RenderFrame()     ← RenderGraph compile → RendererCommandList → Dx12Backend → GPU
```

---

## Known TODOs / In-Progress Work

- Delta time hardcoded at 1/60s
- Jolt job system barrier integration incomplete
- MirrorToPhysicsBodies (ECS→Jolt) commented out — needed for teleport/init
- Vehicle physics: VehicleController component defined, no Jolt implementation
- Animation blending not implemented
- Skeleton index hardcoded to 0
- Render graph is Dx12-specific (should be API-agnostic)
- Resource state tracking should be internal to resources
- TaskGraph largely unused (superseded by flecs)
- InputControllerFeature vehicle handling commented out
