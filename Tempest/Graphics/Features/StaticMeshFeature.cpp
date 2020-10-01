#include <Graphics/Features/StaticMeshFeature.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <Graphics/RendererCommandList.h>

namespace Tempest
{
namespace GraphicsFeature
{

void StaticMesh::GatherData(const World& world, FrameData& frameData)
{
	// TODO: Maybe gather data will be better as a system which is executed directly
	flecs::filter filter = flecs::filter(world.m_EntityWorld)
		.include<Components::Transform>()
		.include<Components::StaticMesh>()
		.include_kind(flecs::MatchAll);

	for (flecs::iter itr : world.m_EntityWorld.filter(filter))
	{
		flecs::column transforms = itr.table_column<Components::Transform>();
		flecs::column staticMeshes = itr.table_column<Components::StaticMesh>();

		for (int row : itr) {
			frameData.StaticMeshes.push_back(FrameData::StaticMeshData{
				staticMeshes[row].Mesh
			});
		}
	}
}

void StaticMesh::GenerateCommands(const FrameData& data, RendererCommandList& commandList)
{
	for (const auto& mesh : data.StaticMeshes)
	{
		RendererCommandDrawStaticMesh command;
		command.Mesh = mesh.Mesh;
		commandList.AddCommand(command);
	}
}

GraphicsFeatureDescription StaticMesh::GetDescription()
{
	GraphicsFeatureDescription desc;
	desc.Id = StaticMesh::ID;
	desc.GatherData = &StaticMesh::GatherData;
	desc.GenerateCommands = &StaticMesh::GenerateCommands;
	return desc;
}
}
}