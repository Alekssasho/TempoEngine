#include <CommonIncludes.h>

#include <Engine.h>
#include <DebugMenu.h>
#include <World/Components/DebugImGuiComponents.h>

#include <imgui.h>

namespace Tempest
{
void DebugWindow::Render()
{
	if (Shown)
	{
		if (ImGui::Begin(Name, &Shown))
		{
			DoRender();

			ImGui::End();
		}
	}
}

struct EntitiesDebugger : DebugWindow
{
	EntitiesDebugger()
	{
		Name = "Entities Debugger";

		Query = gEngine->GetWorld().m_EntityWorld.query_builder()
			.without(flecs::ChildOf, flecs::Flecs).self().up()
			.without(flecs::Module).self().up()
			.without<flecs::Component>().self().up()
			.without(flecs::Observer).self().up()
			.without(flecs::System).self().up()
			.without(flecs::Prefab).self().up()
			//.cached()
			.build();
	}

	virtual void DoRender() override
	{
		Query.each([&](flecs::entity e) {
			//auto entity = m_World.m_EntityWorld.entity(e);
			if (ImGui::TreeNode(e.name().size() > 0 ? e.name().c_str() : "NULL"))
			{
				e.each([&](flecs::id componentId) {
					//auto comp = gEngine->GetWorld().m_EntityWorld.entity(componentId);
					//ImGui::Text(comp.name().c_str());
					auto findItr = g_CompIdToImGuiFunc.find(componentId);
					if (findItr != g_CompIdToImGuiFunc.end())
					{
						findItr->second(e.get_mut(componentId));
					}
				});

				ImGui::TreePop();
			}
		});
	}

	flecs::query<> Query;
};

struct PrefabDebugger : DebugWindow
{
	PrefabDebugger()
	{
		Name = "Prefabs";

		Query = gEngine->GetWorld().m_EntityWorld.query_builder()
			.with(flecs::Prefab).self().up()
			//.cached()
			.build();
	}

	virtual void DoRender() override
	{
		Query.each([&](flecs::entity e) {
			//auto entity = m_World.m_EntityWorld.entity(e);
			if (ImGui::TreeNode(e.name().size() > 0 ? e.name().c_str() : "NULL"))
			{
				ImGui::TreePop();
			}
		});
	}

	flecs::query<> Query;
};

DebugOptions::DebugOptions()
{
	CurrentDeltaTime = 1.0f;
	DeltaTimes = { 0.0f, 0.1f, 0.5f, 1.0f, 2.0f };
}

void Engine::DoDebugMenu()
{
	// TODO: check if we have enabled debug menus and early return

	// Add debug windows if empty
	if (m_DebugOptions.DebugWindows.empty())
	{
		m_DebugOptions.DebugWindows.push_back(eastl::make_unique<EntitiesDebugger>());
		m_DebugOptions.DebugWindows.push_back(eastl::make_unique<PrefabDebugger>());
	}

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("World"))
		{
			ImGui::SeparatorText("Debug Windows");
			for (auto& debugWindowPtr : m_DebugOptions.DebugWindows)
			{
				ImGui::MenuItem(debugWindowPtr->Name, nullptr, &debugWindowPtr->Shown);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Render"))
		{
			ImGui::Checkbox("Show Navigation", &m_DebugOptions.ShowNavigationLanes);
			ImGui::EndMenu();
		}

		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 260.0f);
		ImGui::Text("World Speed:");
		ImGui::SameLine();
		const char* names[] = { "0x", "0.1x", "0.5x", "1x", "2x" };
		auto selectedIndex = eastl::distance(m_DebugOptions.DeltaTimes.begin(), eastl::find(m_DebugOptions.DeltaTimes.begin(), m_DebugOptions.DeltaTimes.end(), m_DebugOptions.CurrentDeltaTime));
		for (uint32_t i = 0; i < 5; ++i)
		{
			if (ImGui::Selectable(names[i], i == selectedIndex, ImGuiSelectableFlags_None, ImVec2(25.0f, 0.0f)))
			{
				m_DebugOptions.CurrentDeltaTime = m_DebugOptions.DeltaTimes[i];
				m_World.m_EntityWorld.set_time_scale(m_DebugOptions.CurrentDeltaTime);
			}
			if (i < 4)
			{
				ImGui::SameLine();
			}
		}

		ImGui::EndMainMenuBar();
	}

	for (auto& debugWindowPtr : m_DebugOptions.DebugWindows)
	{
		debugWindowPtr->Render();
	}
}
}