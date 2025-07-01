#include <CommonIncludes.h>

#include <World/Components/DebugImGuiComponents.h>
#include <World/Camera.h>
#include <World/Components/Components.h>

namespace Tempest
{
eastl::unordered_map<flecs::id_t, void(*)(void*)> g_CompIdToImGuiFunc;

template<>
void ImGuiDebugRender(glm::vec3* v, const char* nameOfMember)
{
	ImGui::InputFloat3(nameOfMember, &v->x);
}

template<>
void ImGuiDebugRender(glm::vec4* v, const char* nameOfMember)
{
    ImGui::InputFloat4(nameOfMember, &v->x);
}

template<>
void ImGuiDebugRender(glm::quat* v, const char* nameOfMember)
{
    ImGui::InputFloat4(nameOfMember, &v->x);
}

template<>
void ImGuiDebugRender(float* v, const char* nameOfMember)
{
    ImGui::InputFloat(nameOfMember, v);
}

template<>
void ImGuiDebugRender(uint32_t* v, const char* nameOfMember)
{
    ImGui::InputInt(nameOfMember, (int*)v);
}

template<>
void ImGuiDebugRender(Camera* v, const char* nameOfMember)
{
}

template<>
void ImGuiDebugRender(CastleFight::Faction* v, const char* nameOfMember)
{
	ImGui::SameLine();
	switch (*v)
	{
	case CastleFight::Faction::Blue:
		ImGui::Text("Blue");
		return;
	case CastleFight::Faction::Red:
		ImGui::Text("Red");
		return;
	}
}

template<>
void ImGuiDebugRender(AnimationState* v, const char* nameOfMember)
{
    ImGui::SameLine();
    switch (*v)
    {
    case AnimationState::Idle:
        ImGui::Text("Idle");
        return;
    case AnimationState::Move:
        ImGui::Text("Move");
        return;
    case AnimationState::Attack:
        ImGui::Text("Attack");
        return;
    }
}

template<>
void ImGuiDebugRender(flecs::entity* v, const char* nameOfMember)
{

}

template<>
void ImGuiDebugRender(Navigation::LaneIterator* v, const char* nameOfMember)
{
}

template<>
void ImGuiDebugRender(physx::PxRigidBody** v, const char* nameOfMember)
{
}

}