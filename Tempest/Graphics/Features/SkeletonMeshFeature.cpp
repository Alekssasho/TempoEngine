#include <CommonIncludes.h>

#include <Graphics/Features/SkeletonMeshFeature.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/Renderer.h>
#include <Graphics/FrameData.h>
#include <World/World.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>
#include <Graphics/RenderGraph.h>

#include <Engine.h>

namespace Tempest
{
namespace GraphicsFeature
{

void SkeletonMesh::Initialize(const World& world, Renderer& renderer)
{
	m_Query = world.m_EntityWorld.query<const Components::Transform, const Components::SkeletonMesh>("Skeleton Mesh Feature Query");
	m_Handle = renderer.RequestPipelineState(PipelineStateDescription{
        "SkeletonMesh",
        "MeshPixel",
		RenderPhase::Main
	});

	m_ShadowHandle = renderer.RequestPipelineState(PipelineStateDescription{
        "SkeletonMesh",
        "MeshPixel",
		RenderPhase::Shadow
	});
}

void SkeletonMesh::GatherData(const World& world, FrameData& frameData)
{
	m_Query.each([&frameData](const Components::Transform& transform, const Components::SkeletonMesh& skeletonMesh) {
		const glm::mat4x4 scale = glm::scale(transform.Scale);
		const glm::mat4x4 rotate = glm::toMat4(transform.Rotation);
		const glm::mat4x4 translate = glm::translate(transform.Position);

		frameData.SkeletonMeshes.push_back(FrameData::SkeletonMeshData{
			skeletonMesh.Mesh,
			translate * rotate * scale,
			uint32_t(frameData.BoneMatrices.size())
		});

		AnimationManager& anim = gEngine->GetAnimation();
		eastl::vector<glm::mat4x4> boneMatrices = skeletonMesh.BoneTransforms;

		// TODO: add proper skeleton index and add start of bone matrices data
		anim.ApplyInverseBindMatrices(0, boneMatrices);

		frameData.BoneMatrices.insert(frameData.BoneMatrices.end(), boneMatrices.begin(), boneMatrices.end());
	});
}

void SkeletonMesh::GenerateCommands(const FrameData& data, RendererCommandList& commandList, const RenderGraphBlackboard& blackboard)
{
	struct GeometryConstants
	{
		glm::mat4x4 worldMatrix;
		uint32_t meshletOffset;
		uint32_t materialIndex;
		uint32_t extraDataIndex;
		uint32_t extraDataStartIndex;
	};
	Dx12::ConstantBufferDataManager& constantDataManager = blackboard.GetConstantDataManager();

	uint32_t extraDataIndex = blackboard.GetRenderer().WriteExtraData(uint32_t(data.FrameIndex), data.BoneMatrices.data(), uint32_t(data.BoneMatrices.size()) * sizeof(glm::mat4x4));

	for (const auto& mesh : data.SkeletonMeshes)
	{
		auto primitiveMeshes = blackboard.GetRenderer().Meshes.GetMeshData(mesh.Mesh, Definition::MeshType_SkeletonMesh);
		for(const auto& meshData : primitiveMeshes)
		{
			GeometryConstants constants;
			constants.worldMatrix = mesh.Transform;
			constants.meshletOffset = meshData.meshlets_offset();
			constants.materialIndex = meshData.material_index();
			constants.extraDataIndex = extraDataIndex;
			constants.extraDataStartIndex = mesh.StartIndexBoneMatrices;

			RendererCommandDrawMeshlet command;
			command.Pipeline = blackboard.GetRenderPhase() == RenderPhase::Main ? m_Handle : m_ShadowHandle;
			command.ParameterViews[size_t(ShaderParameterType::Scene)].ConstantDataOffset = blackboard.GetConstantDataOffset(BlackboardIdentifier{ "SceneData" });
			command.ParameterViews[size_t(ShaderParameterType::Geometry)].ConstantDataOffset = constantDataManager.AddData(constants);
			command.MeshletCount = meshData.meshlets_count();
			commandList.AddCommand(command);
		}
	}
}
}
}
