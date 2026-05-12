#include <CommonIncludes.h>
#include <World/Components/Components.h>
#include <World/Components/DebugImGuiComponents.h>

namespace Tempest
{
void TransformImGuiDebug(void* data)
{
    auto comp = (Components::Transform*)data;
    ImGui::PushID("Transform");
    ImGui::Text("Transform");
    ImGuiDebugRender(&comp->Rotation, "Rotation");
    ImGuiDebugRender(&comp->Position, "Position");
    ImGuiDebugRender(&comp->Scale, "Scale");
    ImGui::PopID();
}
void RectImGuiDebug(void* data)
{
    auto comp = (Components::Rect*)data;
    ImGui::PushID("Rect");
    ImGui::Text("Rect");
    ImGuiDebugRender(&comp->Width, "Width");
    ImGuiDebugRender(&comp->Height, "Height");
    ImGuiDebugRender(&comp->Color, "Color");
    ImGui::PopID();
}
void StaticMeshImGuiDebug(void* data)
{
    auto comp = (Components::StaticMesh*)data;
    ImGui::PushID("StaticMesh");
    ImGui::Text("StaticMesh");
    ImGuiDebugRender(&comp->Mesh, "Mesh");
    ImGui::PopID();
}
void SkeletonMeshImGuiDebug(void* data)
{
    auto comp = (Components::SkeletonMesh*)data;
    ImGui::PushID("SkeletonMesh");
    ImGui::Text("SkeletonMesh");
    ImGuiDebugRender(&comp->Mesh, "Mesh");
    ImGuiDebugRender(&comp->BoneTransforms, "BoneTransforms");
    ImGui::PopID();
}
void AnimationInfoImGuiDebug(void* data)
{
    auto comp = (Components::AnimationInfo*)data;
    ImGui::PushID("AnimationInfo");
    ImGui::Text("AnimationInfo");
    ImGuiDebugRender(&comp->AnimationIndices, "AnimationIndices");
    ImGui::PopID();
}
void AnimationControllerImGuiDebug(void* data)
{
    auto comp = (Components::AnimationController*)data;
    ImGui::PushID("AnimationController");
    ImGui::Text("AnimationController");
    ImGuiDebugRender(&comp->State, "State");
    ImGuiDebugRender(&comp->CurrentTime, "CurrentTime");
    ImGui::PopID();
}
void PhysicsBodyImGuiDebug(void* data)
{
    auto comp = (Components::PhysicsBody*)data;
    ImGui::PushID("PhysicsBody");
    ImGui::Text("PhysicsBody");
    ImGuiDebugRender(&comp->ID, "ID");
    ImGui::PopID();
}
void PrefabPhysicsCreationRequestImGuiDebug(void* data)
{
    auto comp = (Components::PrefabPhysicsCreationRequest*)data;
    ImGui::PushID("PrefabPhysicsCreationRequest");
    ImGui::Text("PrefabPhysicsCreationRequest");
    ImGuiDebugRender(&comp->Index, "Index");
    ImGui::PopID();
}
void LightColorInfoImGuiDebug(void* data)
{
    auto comp = (Components::LightColorInfo*)data;
    ImGui::PushID("LightColorInfo");
    ImGui::Text("LightColorInfo");
    ImGuiDebugRender(&comp->Color, "Color");
    ImGuiDebugRender(&comp->Intensity, "Intensity");
    ImGui::PopID();
}
void CameraControllerImGuiDebug(void* data)
{
    auto comp = (Components::CameraController*)data;
    ImGui::PushID("CameraController");
    ImGui::Text("CameraController");
    ImGuiDebugRender(&comp->CameraData, "CameraData");
    ImGuiDebugRender(&comp->InputMapIndex, "InputMapIndex");
    ImGui::PopID();
}
void VehicleControllerImGuiDebug(void* data)
{
    auto comp = (Components::VehicleController*)data;
    ImGui::PushID("VehicleController");
    ImGui::Text("VehicleController");
    ImGuiDebugRender(&comp->InputMapIndex, "InputMapIndex");
    ImGui::PopID();
}
void FactionImGuiDebug(void* data)
{
    auto comp = (Components::Faction*)data;
    ImGui::PushID("Faction");
    ImGui::Text("Faction");
    ImGuiDebugRender(&comp->FactionFlag, "FactionFlag");
    ImGui::PopID();
}
void FactoryImGuiDebug(void* data)
{
    auto comp = (Components::Factory*)data;
    ImGui::PushID("Factory");
    ImGui::Text("Factory");
    ImGuiDebugRender(&comp->PrefabToSpawn, "PrefabToSpawn");
    ImGuiDebugRender(&comp->TimeToSpawn, "TimeToSpawn");
    ImGuiDebugRender(&comp->CurrentTime, "CurrentTime");
    ImGuiDebugRender(&comp->NumSpawned, "NumSpawned");
    ImGui::PopID();
}
void CastleManagerImGuiDebug(void* data)
{
    auto comp = (Components::CastleManager*)data;
    ImGui::PushID("CastleManager");
    ImGui::Text("CastleManager");
    ImGuiDebugRender(&comp->Castles, "Castles");
    ImGui::PopID();
}
void NavigationDataImGuiDebug(void* data)
{
    auto comp = (Components::NavigationData*)data;
    ImGui::PushID("NavigationData");
    ImGui::Text("NavigationData");
    ImGuiDebugRender(&comp->Points, "Points");
    ImGuiDebugRender(&comp->Lines, "Lines");
    ImGui::PopID();
}
void MovementImGuiDebug(void* data)
{
    auto comp = (Components::Movement*)data;
    ImGui::PushID("Movement");
    ImGui::Text("Movement");
    ImGuiDebugRender(&comp->Velocity, "Velocity");
    ImGui::PopID();
}
void LaneMovementImGuiDebug(void* data)
{
    auto comp = (Components::LaneMovement*)data;
    ImGui::PushID("LaneMovement");
    ImGui::Text("LaneMovement");
    ImGuiDebugRender(&comp->Itr, "Itr");
    ImGuiDebugRender(&comp->NeedsReanchor, "NeedsReanchor");
    ImGui::PopID();
}
void MovementInfoImGuiDebug(void* data)
{
    auto comp = (Components::MovementInfo*)data;
    ImGui::PushID("MovementInfo");
    ImGui::Text("MovementInfo");
    ImGuiDebugRender(&comp->Speed, "Speed");
    ImGui::PopID();
}
void PresenceImGuiDebug(void* data)
{
    auto comp = (Components::Presence*)data;
    ImGui::PushID("Presence");
    ImGui::Text("Presence");
    ImGuiDebugRender(&comp->Radius, "Radius");
    ImGui::PopID();
}
void HealthImGuiDebug(void* data)
{
    auto comp = (Components::Health*)data;
    ImGui::PushID("Health");
    ImGui::Text("Health");
    ImGuiDebugRender(&comp->CurrentHealth, "CurrentHealth");
    ImGuiDebugRender(&comp->MaxHealth, "MaxHealth");
    ImGui::PopID();
}
void AttackInfoImGuiDebug(void* data)
{
    auto comp = (Components::AttackInfo*)data;
    ImGui::PushID("AttackInfo");
    ImGui::Text("AttackInfo");
    ImGuiDebugRender(&comp->DamageAmount, "DamageAmount");
    ImGuiDebugRender(&comp->Range, "Range");
    ImGuiDebugRender(&comp->Speed, "Speed");
    ImGuiDebugRender(&comp->AwarenessRadius, "AwarenessRadius");
    ImGuiDebugRender(&comp->ClashSoundEffect, "ClashSoundEffect");
    ImGui::PopID();
}
void AttackingImGuiDebug(void* data)
{
    auto comp = (Components::Attacking*)data;
    ImGui::PushID("Attacking");
    ImGui::Text("Attacking");
    ImGuiDebugRender(&comp->Target, "Target");
    ImGuiDebugRender(&comp->CurrentTime, "CurrentTime");
    ImGui::PopID();
}
void SoundSourceImGuiDebug(void* data)
{
    auto comp = (Components::SoundSource*)data;
    ImGui::PushID("SoundSource");
    ImGui::Text("SoundSource");
    ImGuiDebugRender(&comp->RequestedSoundEffect, "RequestedSoundEffect");
    ImGui::PopID();
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
            .add(flecs::OnInstantiate, flecs::Inherit);
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
        auto componentId = world.component<Components::AnimationInfo>(Components::AnimationInfo::Name)
            .member<eastl::vector<uint32_t>>("AnimationIndices")
            .add(flecs::OnInstantiate, flecs::Inherit);
        g_CompIdToImGuiFunc[componentId] = &AnimationInfoImGuiDebug;
    }
    {
        auto componentId = world.component<Components::AnimationController>(Components::AnimationController::Name)
            .member<uint32_t>("State")
            .member<float>("CurrentTime")
            ;
        g_CompIdToImGuiFunc[componentId] = &AnimationControllerImGuiDebug;
    }
    {
        auto componentId = world.component<Components::PhysicsBody>(Components::PhysicsBody::Name)
            ;
        g_CompIdToImGuiFunc[componentId] = &PhysicsBodyImGuiDebug;
    }
    {
        auto componentId = world.component<Components::PrefabPhysicsCreationRequest>(Components::PrefabPhysicsCreationRequest::Name)
            .member<uint32_t>("Index")
            ;
        g_CompIdToImGuiFunc[componentId] = &PrefabPhysicsCreationRequestImGuiDebug;
    }
    {
        auto componentId = world.component<Components::LightColorInfo>(Components::LightColorInfo::Name)
            .member<glm::vec3>("Color")
            .member<float>("Intensity")
            ;
        g_CompIdToImGuiFunc[componentId] = &LightColorInfoImGuiDebug;
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
            .add(flecs::OnInstantiate, flecs::Inherit);
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
            .add(flecs::OnInstantiate, flecs::Inherit);
        g_CompIdToImGuiFunc[componentId] = &MovementInfoImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Presence>(Components::Presence::Name)
            .member<float>("Radius")
            .add(flecs::OnInstantiate, flecs::Inherit);
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
            .member<uint32_t>("ClashSoundEffect")
            .add(flecs::OnInstantiate, flecs::Inherit);
        g_CompIdToImGuiFunc[componentId] = &AttackInfoImGuiDebug;
    }
    {
        auto componentId = world.component<Components::Attacking>(Components::Attacking::Name)
            .member<flecs::entity>("Target")
            .member<float>("CurrentTime")
            ;
        g_CompIdToImGuiFunc[componentId] = &AttackingImGuiDebug;
    }
    {
        auto componentId = world.component<Components::SoundSource>(Components::SoundSource::Name)
            .member<uint32_t>("RequestedSoundEffect")
            ;
        g_CompIdToImGuiFunc[componentId] = &SoundSourceImGuiDebug;
    }
    world.component<Tags::Attacks>(Tags::Attacks::Name);
    world.component<Tags::Castle>(Tags::Castle::Name);
    world.component<Tags::Boids>(Tags::Boids::Name);
    world.component<Tags::DirectionalLight>(Tags::DirectionalLight::Name);
    world.component<Tags::SimpleMovement>(Tags::SimpleMovement::Name);
    world.component<Tags::SoundListener>(Tags::SoundListener::Name);
    
}
}