#include <CommonIncludes.h>

#include <Graphics/Features/StaticMeshFeature.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/Renderer.h>
#include <Graphics/FrameData.h>
#include <World/World.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>

namespace Tempest
{
namespace GraphicsFeature
{

void StaticMesh::Initialize(const World& world, Renderer& renderer)
{
	m_Query.Init(world);
	m_Handle = renderer.RequestPipelineState(PipelineStateDescription{
		"StaticMesh",
		RenderPhase::Main
	});

	m_ShadowHandle = renderer.RequestPipelineState(PipelineStateDescription{
		"StaticMesh",
		RenderPhase::Shadow
	});
}

void StaticMesh::GatherData(const World& world, FrameData& frameData)
{
	m_Query.ForEach([&frameData](flecs::entity, Components::Transform& transform, Components::StaticMesh& staticMesh) {
		const glm::mat4x4 scale = glm::scale(transform.Scale);
		const glm::mat4x4 rotate = glm::toMat4(transform.Rotation);
		const glm::mat4x4 translate = glm::translate(transform.Position);

		frameData.StaticMeshes.push_back(FrameData::StaticMeshData{
			staticMesh.Mesh,
			translate * rotate * scale
		});
	});
}

void StaticMesh::GenerateCommands(const FrameData& data, RendererCommandList& commandList, const Renderer& renderer, RenderPhase phase)
{
	struct GeometryConstants
	{
		glm::mat4x4 worldMatrix;
		uint32_t meshletOffset;
		uint32_t materialIndex;
	};
	Dx12::ConstantBufferDataManager& constantDataManager = renderer.GetConstantDataManager();

	for (const auto& mesh : data.StaticMeshes)
	{
		auto primitiveMeshes = renderer.Meshes.GetMeshData(mesh.Mesh);
		for(const auto& meshData : primitiveMeshes)
		{
			GeometryConstants constants;
			constants.worldMatrix = mesh.Transform;
			constants.meshletOffset = meshData.meshlets_offset();
			constants.materialIndex = meshData.material_index();

			RendererCommandDrawMeshlet command;
			command.Pipeline = phase == RenderPhase::Main ? m_Handle : m_ShadowHandle;
			command.ParameterViews[size_t(ShaderParameterType::Scene)].ConstantDataOffset = renderer.GetCurrentSceneConstantDataOffset();
			command.ParameterViews[size_t(ShaderParameterType::Geometry)].ConstantDataOffset = constantDataManager.AddData(constants);
			command.MeshletCount = meshData.meshlets_count();
			commandList.AddCommand(command);
		}
	}
}
}
}
