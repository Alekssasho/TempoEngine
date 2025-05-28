#include <CommonIncludes.h>
#include <World/Components/Components.h>
#include <World/Components/DebugImGuiComponents.h>

namespace Tempest
{
void TransformImGuiDebug(void* data)
{
    auto comp = (Components::Transform*)data;
    ImGui::Text("Transform");
    ImGuiDebugRender(&comp->Rotation, "Rotation");
    ImGuiDebugRender(&comp->Position, "Position");
    ImGuiDebugRender(&comp->Scale, "Scale");
    
}
void RectImGuiDebug(void* data)
{
    auto comp = (Components::Rect*)data;
    ImGui::Text("Rect");
    ImGuiDebugRender(&comp->Width, "Width");
    ImGuiDebugRender(&comp->Height, "Height");
    ImGuiDebugRender(&comp->Color, "Color");
    
}
void StaticMeshImGuiDebug(void* data)
{
    auto comp = (Components::StaticMesh*)data;
    ImGui::Text("StaticMesh");
    ImGuiDebugRender(&comp->Mesh, "Mesh");
    
}
void SkeletonMeshImGuiDebug(void* data)
{
    auto comp = (Components::SkeletonMesh*)data;
    ImGui::Text("SkeletonMesh");
    ImGuiDebugRender(&comp->Mesh, "Mesh");
    ImGuiDebugRender(&comp->BoneTransforms, "BoneTransforms");
    
}
void DynamicPhysicsActorImGuiDebug(void* data)
{
    auto comp = (Components::DynamicPhysicsActor*)data;
    ImGui::Text("DynamicPhysicsActor");
    ImGuiDebugRender(&comp->Actor, "Actor");
    
}
void LightColorInfoImGuiDebug(void* data)
{
    auto comp = (Components::LightColorInfo*)data;
    ImGui::Text("LightColorInfo");
    ImGuiDebugRender(&comp->Color, "Color");
    ImGuiDebugRender(&comp->Intensity, "Intensity");
    
}
void CarPhysicsPartImGuiDebug(void* data)
{
    auto comp = (Components::CarPhysicsPart*)data;
    ImGui::Text("CarPhysicsPart");
    ImGuiDebugRender(&comp->CarActor, "CarActor");
    ImGuiDebugRender(&comp->ShapeIndex, "ShapeIndex");
    
}
void CameraControllerImGuiDebug(void* data)
{
    auto comp = (Components::CameraController*)data;
    ImGui::Text("CameraController");
    ImGuiDebugRender(&comp->CameraData, "CameraData");
    ImGuiDebugRender(&comp->InputMapIndex, "InputMapIndex");
    
}
void VehicleControllerImGuiDebug(void* data)
{
    auto comp = (Components::VehicleController*)data;
    ImGui::Text("VehicleController");
    ImGuiDebugRender(&comp->InputMapIndex, "InputMapIndex");
    
}
void FactionImGuiDebug(void* data)
{
    auto comp = (Components::Faction*)data;
    ImGui::Text("Faction");
    ImGuiDebugRender(&comp->FactionFlag, "FactionFlag");
    
}
void FactoryImGuiDebug(void* data)
{
    auto comp = (Components::Factory*)data;
    ImGui::Text("Factory");
    ImGuiDebugRender(&comp->PrefabToSpawn, "PrefabToSpawn");
    ImGuiDebugRender(&comp->TimeToSpawn, "TimeToSpawn");
    ImGuiDebugRender(&comp->CurrentTime, "CurrentTime");
    ImGuiDebugRender(&comp->NumSpawned, "NumSpawned");
    
}
void CastleManagerImGuiDebug(void* data)
{
    auto comp = (Components::CastleManager*)data;
    ImGui::Text("CastleManager");
    ImGuiDebugRender(&comp->Castles, "Castles");
    
}
void NavigationDataImGuiDebug(void* data)
{
    auto comp = (Components::NavigationData*)data;
    ImGui::Text("NavigationData");
    ImGuiDebugRender(&comp->Points, "Points");
    ImGuiDebugRender(&comp->Lines, "Lines");
    
}
void MovementImGuiDebug(void* data)
{
    auto comp = (Components::Movement*)data;
    ImGui::Text("Movement");
    ImGuiDebugRender(&comp->Velocity, "Velocity");
    
}
void LaneMovementImGuiDebug(void* data)
{
    auto comp = (Components::LaneMovement*)data;
    ImGui::Text("LaneMovement");
    ImGuiDebugRender(&comp->Itr, "Itr");
    
}
void MovementInfoImGuiDebug(void* data)
{
    auto comp = (Components::MovementInfo*)data;
    ImGui::Text("MovementInfo");
    ImGuiDebugRender(&comp->Speed, "Speed");
    
}
void PresenceImGuiDebug(void* data)
{
    auto comp = (Components::Presence*)data;
    ImGui::Text("Presence");
    ImGuiDebugRender(&comp->Radius, "Radius");
    
}
void HealthImGuiDebug(void* data)
{
    auto comp = (Components::Health*)data;
    ImGui::Text("Health");
    ImGuiDebugRender(&comp->CurrentHealth, "CurrentHealth");
    ImGuiDebugRender(&comp->MaxHealth, "MaxHealth");
    
}
void AttackInfoImGuiDebug(void* data)
{
    auto comp = (Components::AttackInfo*)data;
    ImGui::Text("AttackInfo");
    ImGuiDebugRender(&comp->DamageAmount, "DamageAmount");
    ImGuiDebugRender(&comp->Range, "Range");
    ImGuiDebugRender(&comp->Speed, "Speed");
    ImGuiDebugRender(&comp->AwarenessRadius, "AwarenessRadius");
    
}
void AttackingImGuiDebug(void* data)
{
    auto comp = (Components::Attacking*)data;
    ImGui::Text("Attacking");
    ImGuiDebugRender(&comp->Target, "Target");
    ImGuiDebugRender(&comp->CurrentTime, "CurrentTime");
    
}

void RegisterComponents(flecs::world& world)
{
    {
        auto componentId = world.component<Components::Transform>(Components::Transform::Name)
            .member<glm::quat>("Rotation")
            .member<glm::vec3>("Position")
            .member<glm::vec3>("Scale")
            ;
        g_CompIdToImGuiFunc[componentId] = &TransformImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Rect>(Components::Rect::Name)
            .member<float>("Width")
            .member<float>("Height")
            .member<glm::vec4>("Color")
            ;
        g_CompIdToImGuiFunc[componentId] = &RectImGuiDebug;
    }
    {
        auto componentId = world.component<Components::StaticMesh>(Components::StaticMesh::Name)
            .member<uint32_t>("Mesh")
            ;
        g_CompIdToImGuiFunc[componentId] = &StaticMeshImGuiDebug;
    }
    {
        auto componentId = world.component<Components::SkeletonMesh>(Components::SkeletonMesh::Name)
            .member<uint32_t>("Mesh")
            .member<eastl::vector<glm::mat4x4>>("BoneTransforms")
            ;
        g_CompIdToImGuiFunc[componentId] = &SkeletonMeshImGuiDebug;
    }
    {
        auto componentId = world.component<Components::DynamicPhysicsActor>(Components::DynamicPhysicsActor::Name)
            ;
        g_CompIdToImGuiFunc[componentId] = &DynamicPhysicsActorImGuiDebug;
    }
    {
        auto componentId = world.component<Components::LightColorInfo>(Components::LightColorInfo::Name)
            .member<glm::vec3>("Color")
            .member<float>("Intensity")
            ;
        g_CompIdToImGuiFunc[componentId] = &LightColorInfoImGuiDebug;
    }
    {
        auto componentId = world.component<Components::CarPhysicsPart>(Components::CarPhysicsPart::Name)
            ;
        g_CompIdToImGuiFunc[componentId] = &CarPhysicsPartImGuiDebug;
    }
    {
        auto componentId = world.component<Components::CameraController>(Components::CameraController::Name)
            ;
        g_CompIdToImGuiFunc[componentId] = &CameraControllerImGuiDebug;
    }
    {
        auto componentId = world.component<Components::VehicleController>(Components::VehicleController::Name)
            .member<uint32_t>("InputMapIndex")
            ;
        g_CompIdToImGuiFunc[componentId] = &VehicleControllerImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Faction>(Components::Faction::Name)
            .member<uint32_t>("FactionFlag")
            ;
        g_CompIdToImGuiFunc[componentId] = &FactionImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Factory>(Components::Factory::Name)
            .member<flecs::entity>("PrefabToSpawn")
            .member<float>("TimeToSpawn")
            .member<float>("CurrentTime")
            .member<uint32_t>("NumSpawned")
            ;
        g_CompIdToImGuiFunc[componentId] = &FactoryImGuiDebug;
    }
    {
        auto componentId = world.component<Components::CastleManager>(Components::CastleManager::Name)
            .member<eastl::vector<flecs::entity>>("Castles")
            ;
        g_CompIdToImGuiFunc[componentId] = &CastleManagerImGuiDebug;
    }
    {
        auto componentId = world.component<Components::NavigationData>(Components::NavigationData::Name)
            .member<eastl::vector<glm::vec3>>("Points")
            .member<eastl::vector<Navigation::LineData>>("Lines")
            ;
        g_CompIdToImGuiFunc[componentId] = &NavigationDataImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Movement>(Components::Movement::Name)
            .member<glm::vec3>("Velocity")
            ;
        g_CompIdToImGuiFunc[componentId] = &MovementImGuiDebug;
    }
    {
        auto componentId = world.component<Components::LaneMovement>(Components::LaneMovement::Name)
            ;
        g_CompIdToImGuiFunc[componentId] = &LaneMovementImGuiDebug;
    }
    {
        auto componentId = world.component<Components::MovementInfo>(Components::MovementInfo::Name)
            .member<float>("Speed")
            ;
        g_CompIdToImGuiFunc[componentId] = &MovementInfoImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Presence>(Components::Presence::Name)
            .member<float>("Radius")
            ;
        g_CompIdToImGuiFunc[componentId] = &PresenceImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Health>(Components::Health::Name)
            .member<float>("CurrentHealth")
            .member<float>("MaxHealth")
            ;
        g_CompIdToImGuiFunc[componentId] = &HealthImGuiDebug;
    }
    {
        auto componentId = world.component<Components::AttackInfo>(Components::AttackInfo::Name)
            .member<float>("DamageAmount")
            .member<float>("Range")
            .member<float>("Speed")
            .member<float>("AwarenessRadius")
            ;
        g_CompIdToImGuiFunc[componentId] = &AttackInfoImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Attacking>(Components::Attacking::Name)
            .member<flecs::entity>("Target")
            .member<float>("CurrentTime")
            ;
        g_CompIdToImGuiFunc[componentId] = &AttackingImGuiDebug;
    }
    world.component<Tags::Attacks>(Tags::Attacks::Name);
    world.component<Tags::Castle>(Tags::Castle::Name);
    world.component<Tags::Boids>(Tags::Boids::Name);
    world.component<Tags::DirectionalLight>(Tags::DirectionalLight::Name);
    world.component<Tags::SimpleMovement>(Tags::SimpleMovement::Name);
    
}
}