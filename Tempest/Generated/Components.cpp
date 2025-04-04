#include <CommonIncludes.h>
#include <World/Components/Components.h>

namespace Tempest
{
void RegisterComponents(flecs::world& world)
{
    world.component<Components::Transform>(Components::Transform::Name)
        .member<glm::quat>("Rotation")
        .member<glm::vec3>("Position")
        .member<glm::vec3>("Scale")
        ;
    world.component<Components::Rect>(Components::Rect::Name)
        .member<float>("Width")
        .member<float>("Height")
        .member<glm::vec4>("Color")
        ;
    world.component<Components::StaticMesh>(Components::StaticMesh::Name)
        .member<uint32_t>("Mesh")
        ;
    world.component<Components::DynamicPhysicsActor>(Components::DynamicPhysicsActor::Name)
        ;
    world.component<Components::LightColorInfo>(Components::LightColorInfo::Name)
        .member<glm::vec3>("Color")
        .member<float>("Intensity")
        ;
    world.component<Components::CarPhysicsPart>(Components::CarPhysicsPart::Name)
        ;
    world.component<Components::CameraController>(Components::CameraController::Name)
        ;
    world.component<Components::VehicleController>(Components::VehicleController::Name)
        .member<uint32_t>("InputMapIndex")
        ;
    world.component<Components::Faction>(Components::Faction::Name)
        .member<uint32_t>("FactionFlag")
        ;
    world.component<Components::Factory>(Components::Factory::Name)
        .member<flecs::entity>("PrefabToSpawn")
        .member<float>("TimeToSpawn")
        .member<float>("CurrentTime")
        ;
    world.component<Components::CastleManager>(Components::CastleManager::Name)
        .member<eastl::vector<flecs::entity>>("Castles")
        ;
    world.component<Components::NavigationData>(Components::NavigationData::Name)
        .member<eastl::vector<glm::vec3>>("Points")
        .member<eastl::vector<Navigation::LineData>>("Lines")
        ;
    world.component<Components::Movement>(Components::Movement::Name)
        .member<glm::vec3>("Velocity")
        ;
    world.component<Components::LaneMovement>(Components::LaneMovement::Name)
        ;
    world.component<Components::MovementInfo>(Components::MovementInfo::Name)
        .member<float>("Speed")
        ;
    world.component<Tags::Attacks>(Tags::Attacks::Name);
    world.component<Tags::Castle>(Tags::Castle::Name);
    world.component<Tags::Boids>(Tags::Boids::Name);
    world.component<Tags::DirectionalLight>(Tags::DirectionalLight::Name);
    world.component<Tags::SimpleMovement>(Tags::SimpleMovement::Name);
    
}
}