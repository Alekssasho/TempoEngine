#include <CommonIncludes.h>

#include <Engine.h>
#include <DebugMenu.h>
#include <World/Components/DebugImGuiComponents.h>
#include <World/GameplayFeatures/SpaceLocation.h>

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

struct SpaceLocationDebugger : DebugWindow
{
	SpaceLocationDebugger()
	{
		Name = "Space Location Debugger";
	}

	virtual void DoRender() override
	{
		auto spaceData = gEngine->GetWorld().m_EntityWorld.get<GameplayFeatures::SpaceLocation::SpaceData>();

		auto drawList = ImGui::GetWindowDrawList();

		auto windowSize = ImGui::GetContentRegionAvail();
		auto windowPos = ImGui::GetCursorScreenPos();

		// Make background grid
		for (uint32_t i = 0; i <= GameplayFeatures::SpaceLocation::s_WorldCellDivisions; ++i)
		{
			const auto horizontalLineHeight = windowPos.y + (i * (windowSize.y / GameplayFeatures::SpaceLocation::s_WorldCellDivisions));
			drawList->AddLine(ImVec2(windowPos.x, horizontalLineHeight), ImVec2(windowPos.x + windowSize.x, horizontalLineHeight), IM_COL32_WHITE);

			const auto verticalLineHeight = windowPos.x + (i * (windowSize.x / GameplayFeatures::SpaceLocation::s_WorldCellDivisions));
			drawList->AddLine(ImVec2(verticalLineHeight, windowPos.y), ImVec2(verticalLineHeight, windowPos.y + windowSize.y), IM_COL32_WHITE);
		}

		const auto cellWindowSize = glm::vec2(
			windowSize.x / float(GameplayFeatures::SpaceLocation::s_WorldCellDivisions),
			windowSize.y / float(GameplayFeatures::SpaceLocation::s_WorldCellDivisions)
		);

		const auto cellWorldSize = glm::vec2(
			GameplayFeatures::SpaceLocation::s_WorldSize / float(GameplayFeatures::SpaceLocation::s_WorldCellDivisions),
			GameplayFeatures::SpaceLocation::s_WorldSize / float(GameplayFeatures::SpaceLocation::s_WorldCellDivisions)
		);

		// Draw each entity from every cell
		for (uint32_t cellIndex = 0; cellIndex < spaceData->Cells.size(); ++cellIndex)
		{
			uint32_t cellX = cellIndex % GameplayFeatures::SpaceLocation::s_WorldCellDivisions;
			uint32_t cellY = cellIndex / GameplayFeatures::SpaceLocation::s_WorldCellDivisions;

			auto cellWorldPos = glm::vec2(
				float(cellX * cellWorldSize.x) - GameplayFeatures::SpaceLocation::s_WorldSize / 2.0f,
				float(cellY * cellWorldSize.y) - GameplayFeatures::SpaceLocation::s_WorldSize / 2.0f
			);

			auto& cellData = spaceData->Cells[cellIndex];

			for (uint32_t entityIndex = 0; entityIndex < cellData.Count; ++entityIndex)
			{
				if (!spaceData->Entities[entityIndex + cellData.StartIndex].is_alive())
					continue;
				auto& faction = spaceData->Entities[entityIndex + cellData.StartIndex].get<Components::Faction>()->FactionFlag;
				auto factionColor = faction == CastleFight::Faction::Blue ? IM_COL32(0, 0, 255, 255) : IM_COL32(255, 0, 0, 255);

				auto& entityPosition = spaceData->Entities[entityIndex + cellData.StartIndex].get<Components::Transform>()->Position;
				auto localPosInCell = glm::vec2(entityPosition.x - cellWorldPos.x, entityPosition.z - cellWorldPos.y) * (cellWindowSize / cellWorldSize);

				drawList->AddCircle(ImVec2(localPosInCell.x + windowPos.x + cellX * cellWindowSize.x, localPosInCell.y + windowPos.y + cellY * cellWindowSize.y), 10, factionColor);
			}
		}
	}
};

DebugOptions::DebugOptions()
{
	CurrentDeltaTime = 1.0f;
	DeltaTimes = { 0.0f, 0.1f, 0.5f, 1.0f, 2.0f, 5.0f };
}

void Engine::DoDebugMenu()
{
	// TODO: check if we have enabled debug menus and early return

	// Add debug windows if empty
	if (m_DebugOptions.DebugWindows.empty())
	{
		m_DebugOptions.DebugWindows.push_back(eastl::make_unique<EntitiesDebugger>());
		m_DebugOptions.DebugWindows.push_back(eastl::make_unique<PrefabDebugger>());
		m_DebugOptions.DebugWindows.push_back(eastl::make_unique<SpaceLocationDebugger>());
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
            ImGui::Checkbox("Show Skeletons", &m_DebugOptions.ShowSkeletons);
			ImGui::EndMenu();
		}

		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 280.0f);
		ImGui::Text("World Speed:");
		ImGui::SameLine();
		const char* names[] = { "0x", "0.1x", "0.5x", "1x", "2x", "5x"};
		auto selectedIndex = eastl::distance(m_DebugOptions.DeltaTimes.begin(), eastl::find(m_DebugOptions.DeltaTimes.begin(), m_DebugOptions.DeltaTimes.end(), m_DebugOptions.CurrentDeltaTime));
		for (uint32_t i = 0; i < 6; ++i)
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