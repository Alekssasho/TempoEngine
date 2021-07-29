#include <CommonIncludes.h>

#include <Graphics/Features/StaticMeshFeature.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/Renderer.h>
#include <Graphics/FrameData.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <World/EntityQueryImpl.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>

namespace Tempest
{
namespace GraphicsFeature
{

void StaticMesh::Initialize(const World& world, Renderer& renderer)
{
	m_Query.Init<Components::Transform, Components::StaticMesh>(world);
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
	int archetypeCount = m_Query.GetMatchedArchetypesCount();
	for (int i = 0; i < archetypeCount; ++i)
	{
		auto [_, iter] = m_Query.GetIterForAchetype(i);
		Components::Transform* transforms = ecs_column(&iter, Components::Transform, 1);
		Components::StaticMesh* staticMeshes = ecs_column(&iter, Components::StaticMesh, 2);
		for (int row = 0; row < iter.count; ++row)
		{
			const glm::mat4x4 scale = glm::scale(transforms[row].Scale);
			const glm::mat4x4 rotate = glm::toMat4(transforms[row].Rotation);
			const glm::mat4x4 translate = glm::translate(transforms[row].Position);

			frameData.StaticMeshes.push_back(FrameData::StaticMeshData{
				staticMeshes[row].Mesh,
				translate * rotate * scale
			});
		}
	}
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
