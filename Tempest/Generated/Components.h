#pragma once

namespace Tempest
{
namespace Components
{

struct Transform
{
    glm::quat Rotation;
    glm::vec3 Position;
    glm::vec3 Scale;
    static constexpr const char* Name = "Transform";
};
struct Rect
{
    float Width;
    float Height;
    glm::vec4 Color;
    static constexpr const char* Name = "Rect";
};
struct StaticMesh
{
    MeshHandle Mesh;
    static constexpr const char* Name = "StaticMesh";
};
struct SkeletonMesh
{
    MeshHandle Mesh;
    eastl::vector<glm::mat4x4> BoneTransforms;
    static constexpr const char* Name = "SkeletonMesh";
};
struct AnimationController
{
    uint32_t AnimationIndex;
    float CurrentTime;
    static constexpr const char* Name = "AnimationController";
};
struct DynamicPhysicsActor
{
    physx::PxRigidBody* Actor;
    static constexpr const char* Name = "DynamicPhysicsActor";
};
struct LightColorInfo
{
    glm::vec3 Color;
    float Intensity;
    static constexpr const char* Name = "LightColorInfo";
};
struct CarPhysicsPart
{
    physx::PxRigidBody* CarActor;
    uint32_t ShapeIndex;
    static constexpr const char* Name = "CarPhysicsPart";
};
struct CameraController
{
    Camera CameraData;
    uint32_t InputMapIndex;
    static constexpr const char* Name = "CameraController";
};
struct VehicleController
{
    uint32_t InputMapIndex;
    static constexpr const char* Name = "VehicleController";
};
struct Faction
{
    CastleFight::Faction FactionFlag;
    static constexpr const char* Name = "Faction";
};
struct Factory
{
    flecs::entity PrefabToSpawn;
    float TimeToSpawn;
    float CurrentTime;
    uint32_t NumSpawned;
    static constexpr const char* Name = "Factory";
};
struct CastleManager
{
    eastl::vector<flecs::entity> Castles;
    static constexpr const char* Name = "CastleManager";
};
struct NavigationData
{
    eastl::vector<glm::vec3> Points;
    eastl::vector<Navigation::LineData> Lines;
    static constexpr const char* Name = "NavigationData";
};
struct Movement
{
    glm::vec3 Velocity;
    static constexpr const char* Name = "Movement";
};
struct LaneMovement
{
    Navigation::LaneIterator Itr;
    static constexpr const char* Name = "LaneMovement";
};
struct MovementInfo
{
    float Speed;
    static constexpr const char* Name = "MovementInfo";
};
struct Presence
{
    float Radius;
    static constexpr const char* Name = "Presence";
};
struct Health
{
    float CurrentHealth;
    float MaxHealth;
    static constexpr const char* Name = "Health";
};
struct AttackInfo
{
    float DamageAmount;
    float Range;
    float Speed;
    float AwarenessRadius;
    static constexpr const char* Name = "AttackInfo";
};
struct Attacking
{
    flecs::entity Target;
    float CurrentTime;
    static constexpr const char* Name = "Attacking";
};
}
}

namespace Tempest
{
namespace Tags
{

struct Attacks
{
    static constexpr const char* Name = "Attacks";
};
struct Castle
{
    static constexpr const char* Name = "Castle";
};
struct Boids
{
    static constexpr const char* Name = "Boids";
};
struct DirectionalLight
{
    static constexpr const char* Name = "DirectionalLight";
};
struct SimpleMovement
{
    static constexpr const char* Name = "SimpleMovement";
};
}
}