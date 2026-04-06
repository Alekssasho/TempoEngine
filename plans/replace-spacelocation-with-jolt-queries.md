# Plan: Replace Custom Spatial Grid with Jolt Physics Collision Queries

## Context

The game's combat system (target acquisition, attack range checks) currently uses a custom 10x10 spatial grid (`SpaceLocation`) rebuilt every frame. NPCs already have Jolt physics bodies (kinematic spheres), but they're only used for movement. This change replaces the custom grid with Jolt's built-in broadphase queries, reducing custom code and leveraging the physics engine's optimized spatial data structures.

## Approach: On-Demand BroadPhaseQuery::CollideSphere

Use Jolt's `CollideSphere` to find nearby bodies when acquiring targets. This maps directly to the current pattern (query a sphere at position with AwarenessRadius, get back nearby entities). No sensor bodies or contact listeners needed — the query is read-only and thread-safe for multi-threaded ECS systems.

## Changes

### 1. PhysicsManager — Add query API and UserData support

**`Tempest/Physics/PhysicsManager.h`** — Add 2 new methods:
```cpp
void QuerySphere(glm::vec3 center, float radius, eastl::vector<JPH::BodyID>& outBodies) const;
uint64_t GetBodyUserData(JPH::BodyID id) const;
```

**`Tempest/Physics/PhysicsManager.cpp`**:
- Modify `CreateBodyFromPrefabScene` to accept `uint64_t userData` parameter and set `settings.mUserData = userData` before creating the body. This is the critical link — without it, query results can't be mapped back to flecs entities.
- Implement `QuerySphere` using `m_Impl->System.GetBroadPhaseQuery().CollideSphere(...)` with a `DefaultObjectLayerFilter` targeting only `Dynamic` layer.
- Implement `GetBodyUserData` using `GetBodyInterfaceNoLock().GetUserData()`.
- New includes: `Jolt/Physics/Collision/CollisionCollectorImpl.h`, `Jolt/Physics/Collision/ObjectLayer.h`

### 2. PhysicsFeature — Pass entity ID as UserData

**`Tempest/World/GameplayFeatures/PhysicsFeature.h`** — In "Init Prefab Physics" observer (line 55), change:
```cpp
// Before:
JPH::BodyID newId = gEngine->GetPhysics().CreateBodyFromPrefabScene(request.Index, transform);
// After:
JPH::BodyID newId = gEngine->GetPhysics().CreateBodyFromPrefabScene(request.Index, transform, e.id());
```

### 3. BattleFeature — Replace SpaceLocation with Jolt queries

**`Tempest/World/GameplayFeatures/BattleFeature.h`**:

**"Acquire Target" system** — Add `PhysicsBody` to query terms. Replace `SpaceLocation::ForEachCellTouched` with `gEngine->GetPhysics().QuerySphere(position, awarenessRadius, bodies)`. Resolve entities via `GetBodyUserData`. Skip self by comparing BodyID. Keep faction filtering in application code.

**"Battle Initial" system** — Remove SpaceLocation dependency entirely. The system already has `att.Target` — just do a direct `glm::distance2` check against the single target entity instead of iterating through a spatial cell. This is actually a simplification.

Remove `#include <World/GameplayFeatures/SpaceLocation.h>`.

### 4. Remove SpaceLocation

- **`Tempest/World/World.cpp`** — Remove include (line 13) and feature registration (line 180)
- **`Tempest/DebugMenu.cpp`** — Remove include (line 6), `SpaceLocationDebugger` struct (lines 94-157), and its registration (line 174)
- **`Tempest/World/GameplayFeatures/SpaceLocation.h`** — Delete file

### 5. No changes needed

- **`SoldierMovementFeature.h`** — Uses direct `glm::distance2` against `att.Target`, no SpaceLocation dependency
- **`DataSchemas/Components.txt`** — `Presence` component stays (may be useful elsewhere, or can be removed later)
- **`PhysicsConstants.h`** — No new layers needed; faction filtering stays in game code

## Thread Safety Notes

- `BroadPhaseQuery::CollideSphere` is read-only and thread-safe — safe for multi-threaded ECS systems
- Each thread creates its own stack-local `AllHitCollisionCollector`
- Body destruction happens in observers (main thread), not during multi-threaded PreUpdate phase
- Pre-existing data race on `get_mut<Health>` from multiple attackers is unchanged by this work

## Verification

1. Build the C++ solution in Visual Studio (Debug config)
2. Run Spark with the CastleFight level
3. Verify soldiers still: acquire targets, move toward enemies, stop in range, deal damage, die when health depletes
4. Check that both factions fight correctly (no friendly fire, no missing targets)
5. Verify the SpaceLocationDebugger is gone from the debug menu without crashes
