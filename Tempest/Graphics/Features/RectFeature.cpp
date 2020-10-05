#include <Graphics/Features/RectFeature.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <Graphics/RendererCommandList.h>
#include <World/EntityQueryImpl.h>
#include <Graphics/Renderer.h>

namespace Tempest
{
namespace GraphicsFeature
{

void Rects::Initialize(const World& world, Renderer& renderer)
{
	m_Query.Init<Components::Transform, Components::Rect>(world);
	m_Handle = renderer.RequestPipelineState(PipelineStateDescription{
		"Rects"
	});
}

void Rects::GatherData(const World& world, FrameData& frameData)
{
	int archetypeCount = m_Query.GetMatchedArchetypesCount();
	for (int i = 0; i < archetypeCount; ++i)
	{
		auto [_, iter] = m_Query.GetIterForAchetype(i);
		Components::Transform* transforms = ecs_column(&iter, Components::Transform, 1);
		Components::Rect* rects = ecs_column(&iter, Components::Rect, 2);
		for (int row = 0; row < iter.count; ++row)
		{
			frameData.Rects.push_back(RectData{
				transforms[row].Position.x,
				transforms[row].Position.y,
				rects[row].width,
				rects[row].height,
				rects[row].color,
			});
		}
	}
}

void Rects::GenerateCommands(const FrameData& data, RendererCommandList& commandList)
{
	for (const auto& rect : data.Rects)
	{
		RendererCommandDrawRect command;
		command.Pipeline = m_Handle;
		command.Data = rect;
		commandList.AddCommand(command);
	}
}
}
}