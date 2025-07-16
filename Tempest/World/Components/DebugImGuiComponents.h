#pragma once

#include <World/World.h>
#include <imgui.h>

namespace Tempest
{
extern eastl::unordered_map<flecs::id_t, void(*)(void*)> g_CompIdToImGuiFunc;

template<typename T>
void ImGuiDebugRender(T* data, const char* nameOfMember);

template<typename T>
void ImGuiDebugRender(eastl::vector<T>* data, const char* nameOfMember)
{
}
}