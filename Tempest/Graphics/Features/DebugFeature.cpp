#include <CommonIncludes.h>

#include <Engine.h>
#include <Graphics/Features/DebugFeature.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/FrameData.h>
#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>
#include <Graphics/RenderGraph.h>

namespace Tempest
{
namespace GraphicsFeature
{

void Debug::Initialize(const World& world, Renderer& renderer)
{
	m_Handle = renderer.RequestPipelineState(PipelineStateDescription{
		"DebugShape",
		RenderPhase::Main
	});
}

void Debug::GatherData(const World& world, FrameData& frameData)
{
	if (gEngine->GetDebug().ShowNavigationLanes)
	{
		const Components::NavigationData* navData = world.m_EntityWorld.get<Components::NavigationData>();
		for (const auto& line : navData->Lines)
		{
			for (uint32_t i = 1; i < line.Count; ++i)
			{
				const auto& prevPoint = navData->Points[line.StartIndex + i - 1];
				const auto& nextPoint = navData->Points[line.StartIndex + i];

				const glm::mat4x4 scale = glm::scale(glm::vec3(1.0f, 1.0f, glm::distance(prevPoint, nextPoint)));
				const glm::mat4x4 rotate = glm::toMat4(glm::rotation(sForwardDirection, glm::normalize(nextPoint - prevPoint)));
				const glm::mat4x4 translate = glm::translate(prevPoint + 1.0f * sUpDirection);

				frameData.DebugRects.emplace_back(
					translate * rotate * scale,
					glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)
				);
			}
		}
	}
}

void Debug::GenerateCommands(const FrameData& data, RendererCommandList& commandList, const RenderGraphBlackboard& blackboard)
{
	if (blackboard.GetRenderPhase() != RenderPhase::Main)
	{
		return;
	}

	Dx12::ConstantBufferDataManager& constantDataManager = blackboard.GetConstantDataManager();

	for (const auto& rect : data.DebugRects)
	{
		RendererCommandDrawMeshlet command;
		command.Pipeline = m_Handle;
		command.ParameterViews[size_t(ShaderParameterType::Scene)].ConstantDataOffset = blackboard.GetConstantDataOffset(BlackboardIdentifier{ "SceneData" });
		command.ParameterViews[size_t(ShaderParameterType::Geometry)].ConstantDataOffset = constantDataManager.AddData(rect);
		command.MeshletCount = 1;
		commandList.AddCommand(command);
	}
}
}
}