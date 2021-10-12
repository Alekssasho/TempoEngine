#include <CommonIncludes.h>

#include <Graphics/Features/RectFeature.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/FrameData.h>
#include <World/EntityQueryImpl.h>
#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>

namespace Tempest
{
namespace GraphicsFeature
{

void Rects::Initialize(const World& world, Renderer& renderer)
{
	m_Query.Init(world);
	m_Handle = renderer.RequestPipelineState(PipelineStateDescription{
		"Rects"
	});
}

void Rects::GatherData(const World& world, FrameData& frameData)
{
	m_Query.ForEach([&frameData](flecs::entity, Components::Transform& transform, Components::Rect& rect) {
		frameData.Rects.push_back(RectData{
			transform.Position.x,
			transform.Position.y,
			rect.width,
			rect.height,
			rect.color,
		});
	});
}

void Rects::GenerateCommands(const FrameData& data, RendererCommandList& commandList, const Renderer& renderer, RenderPhase phase)
{
	Dx12::ConstantBufferDataManager& constantDataManager = renderer.GetConstantDataManager();
	for (const auto& rect : data.Rects)
	{
		RendererCommandDrawInstanced command;
		command.Pipeline = m_Handle;
		command.ParameterViews[size_t(ShaderParameterType::Scene)].ConstantDataOffset = renderer.GetCurrentSceneConstantDataOffset();
		command.ParameterViews[size_t(ShaderParameterType::Geometry)].ConstantDataOffset = constantDataManager.AddData(rect);
		command.VertexCountPerInstance = 4;
		command.InstanceCount = 1;
		commandList.AddCommand(command);
	}
}
}
}