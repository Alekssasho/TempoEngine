# ECS-Friendly Physics Collision Handling

## Context

Physics libraries like Jolt use **callbacks** (`ContactListener`) that fire on worker threads during the physics step. This is fundamentally at odds with ECS, where data flows through systems in well-defined phases. We need a bridge that collects collision events thread-safely during the physics step, then exposes them as data that ECS systems can consume in later phases.

## Approach: Thread-Safe Event Buffer (collect-then-distribute)

Follows the same pattern as `SpaceLocation` (build data in one phase, consume in later phases). No per-entity collision components — that would cause archetype churn as entities gain/lose components every frame.

### Why not alternatives?
- **Per-entity component**: Archetype churn — entities gaining/losing collision components each frame degrades flecs
- **Flecs relationships** (`CollidingWith`): Same churn problem, relationships are for stable associations
- **No `Persisted` events**: Games rarely need "still touching" — `Added`/`Removed` pairs cover damage-on-hit and trigger enter/exit. Persisted would flood the buffer with resting contacts.

---

## Phase 1: Prerequisite — Store Entity ID as Body User Data

`CreateBodyFromPrefabScene` must store the entity ID as Jolt body user data so the contact listener can map bodies back to entities. Currently only cooked initial scene bodies have user data (set at cook time, read by `PatchWorldComponents`).

### Files
- `Tempest/Physics/PhysicsManager.h` — Add `flecs::entity_t entityId` parameter to `CreateBodyFromPrefabScene`
- `Tempest/Physics/PhysicsManager.cpp` — Set `settings.mUserData = entityId` before creating body
- `Tempest/World/GameplayFeatures/PhysicsFeature.h` — Pass entity ID in "Init Prefab Physics" observer

### Verification
- Build and run. Existing physics behavior unchanged.
- Breakpoint in `CreateBodyFromPrefabScene` to confirm user data is set.

---

## Phase 2: Contact Event Buffer

Create the thread-safe data structures for collecting collision events.

### New File: `Tempest/Physics/ContactEvents.h`

```cpp
enum class ContactEventType : uint8_t { Added, Removed };

struct ContactEvent {
    flecs::entity_t Entity1;
    flecs::entity_t Entity2;
    ContactEventType Type;
    glm::vec3 ContactPosition;   // world-space, from manifold
    glm::vec3 ContactNormal;     // body1 -> body2
    float PenetrationDepth;
};

struct ContactEventBuffer {
    void Push(const ContactEvent& event);               // thread-safe (mutex)
    void SwapBuffers();                                  // single-threaded, after physics step
    eastl::span<const ContactEvent> GetEvents() const;   // read-only, single-threaded
private:
    eastl::vector<ContactEvent> m_WriteBuffer;
    eastl::vector<ContactEvent> m_ReadBuffer;
    std::mutex m_WriteMutex;
};
```

### Design Notes
- **Mutex is fine.** Jolt worker count is bounded, `Push` is a fast vector append. If profiling shows contention, per-thread buffers with post-step merge is a straightforward upgrade.
- **Double buffering**: Write buffer fills during `PhysicsSystem::Update()`. After it returns, `SwapBuffers()` moves contents to read buffer. ECS systems read from stable, non-contended read buffer.

### Verification
- Build. No runtime changes yet — buffer exists but isn't wired up.

---

## Phase 3: Contact Listener + PhysicsManager Integration

Wire the event buffer into Jolt's contact listener and expose events to the rest of the engine.

### Internal (PhysicsManager.cpp)

Add `TempestContactListener` alongside existing pimpl types:

```cpp
struct TempestContactListener final : public JPH::ContactListener {
    ContactEventBuffer* Buffer;
    JPH::BodyInterface* BodyInterface;

    OnContactValidate(...)  -> AcceptAllContactsForThisBodyPair
    OnContactAdded(...)     -> Push ContactEvent{Added}, entity IDs from body.GetUserData()
    OnContactPersisted(...) -> empty (intentional)
    OnContactRemoved(...)   -> Push ContactEvent{Removed}, entity IDs from BodyInterface->GetUserData()
};
```

`OnContactRemoved` only receives `SubShapeIDPair` (body IDs, no body refs), so we use `BodyInterface::GetUserData()` which is thread-safe.

### PhysicsImpl Changes

Add to `PhysicsImpl`:
```cpp
ContactEventBuffer ContactBuffer;
TempestContactListener ContactListener;
```

In constructor, after `System.Init()`:
```cpp
ContactListener.Buffer = &ContactBuffer;
ContactListener.BodyInterface = &System.GetBodyInterfaceNoLock();
System.SetContactListener(&ContactListener);
```

### PhysicsManager::Update Changes

```cpp
void PhysicsManager::Update(float deltaTime) {
    m_Impl->System.Update(...);
    m_Impl->ContactBuffer.SwapBuffers();  // make events available to ECS
    // ... existing debug recorder code ...
}
```

### PhysicsManager.h — New Public API

```cpp
eastl::span<const ContactEvent> GetContactEvents() const;
```

### Verification
- Build and run with a level that has dynamic physics bodies (e.g., CastleFight)
- Add temporary log in `OnContactAdded` to confirm events fire
- Verify no crashes, no performance regression (Tracy)

---

## Phase 4: Sensor Tag

Add a `Sensor` tag to the component DSL for trigger zones (overlap-only, no physics response).

### Files
- `DataSchemas/Components.txt` — Add `Tag Sensor;`
- Rebuild code generator to regenerate `Tempest/Generated/Components.h/.cpp`

Jolt handles the physics side via `BodyCreationSettings::mIsSensor = true` at cook time. Game systems use this tag to distinguish collision semantics when consuming contact events.

### Verification
- Build. Tag exists in generated code.

---

## Phase 5: Example Consumer System

Add a reference system that demonstrates how to consume contact events from ECS.

### Consumption Pattern (in any GameplayFeature)

```cpp
world.m_EntityWorld.system("Contact Damage")
    .kind(flecs::PostUpdate)
    .run([](flecs::iter& it) {
        auto events = gEngine->GetPhysics().GetContactEvents();
        for (const auto& event : events) {
            if (event.Type != ContactEventType::Added) continue;
            flecs::entity e1(it.world(), event.Entity1);
            flecs::entity e2(it.world(), event.Entity2);
            // game logic: apply damage, trigger effects, etc.
        }
    });
```

No intermediary ECS system needed — data is available after `PhysicsManager::Update` returns in `OnUpdate`.

### Execution Order

```
PreUpdate:    Kinematic Movement, CopyTransformToBody
OnUpdate:     Physics Step -> ContactListener fills buffer -> SwapBuffers()
PostUpdate:   CopyTransformFromBody
              Game systems read GetContactEvents() (damage, triggers, etc.)
              Health check / entity destruction
```

---

## Files Summary

| File | Phase | Action |
|------|-------|--------|
| `Tempest/Physics/PhysicsManager.h` | 1, 3 | Modify — update CreateBodyFromPrefabScene signature, add GetContactEvents() |
| `Tempest/Physics/PhysicsManager.cpp` | 1, 3 | Modify — user data in body creation, add ContactListener, SwapBuffers |
| `Tempest/World/GameplayFeatures/PhysicsFeature.h` | 1 | Modify — pass entity ID in observer |
| `Tempest/Physics/ContactEvents.h` | 2 | Create — ContactEvent, ContactEventType, ContactEventBuffer |
| `DataSchemas/Components.txt` | 4 | Modify — add `Tag Sensor;` |
