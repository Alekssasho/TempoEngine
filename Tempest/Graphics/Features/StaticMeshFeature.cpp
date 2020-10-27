#include <Graphics/Features/StaticMeshFeature.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/Renderer.h>
#include <Graphics/FrameData.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <World/EntityQueryImpl.h>

namespace Tempest
{
namespace GraphicsFeature
{

void StaticMesh::Initialize(const World& world, Renderer& renderer)
{
	m_Query.Init<Components::Transform, Components::StaticMesh>(world);
	m_Handle = renderer.RequestPipelineState(PipelineStateDescription{
		"StaticMesh"
	});
}

void StaticMesh::GatherData(const World& world, FrameData& frameData)
{
	int archetypeCount = m_Query.GetMatchedArchetypesCount();
	for (int i = 0; i < archetypeCount; ++i)
	{
		auto [_, iter] = m_Query.GetIterForAchetype(i);
		Components::Transform* transforms = ecs_column(&iter, Components::Transform, 1);
		Components::StaticMesh* staticMeshes = ecs_column(&iter, Components::StaticMesh, 2);
		for (int row = 0; row < iter.count; ++row)
		{
			frameData.StaticMeshes.push_back(FrameData::StaticMeshData{
				staticMeshes[row].Mesh
			});
		}
	}
}

void StaticMesh::GenerateCommands(const FrameData& data, RendererCommandList& commandList, const Renderer& renderer)
{
	struct GeometryConstants
	{
		uint32_t vertexBufferIndex;
		uint32_t vertexBufferOffset;
	};

	for (const auto& mesh : data.StaticMeshes)
	{
		MeshManager::MeshData meshData = renderer.Meshes.GetMeshData(mesh.Mesh);

		GeometryConstants constants;
		constants.vertexBufferIndex = 0;// meshData.VertexBuffer; // TODO: This should be an index of sort
		constants.vertexBufferOffset = meshData.OffsetInVertexBuffer;

		RendererCommandDrawInstanced command;
		command.Pipeline = m_Handle;
		command.ParameterView.GeometryConstantDataOffset = commandList.AddConstantData(constants);
		command.VertexCountPerInstance = meshData.VertexCount;
		command.InstanceCount = 1;
		commandList.AddCommand(command);
	}
}
}
}
