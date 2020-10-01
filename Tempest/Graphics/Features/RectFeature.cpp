#include <Graphics/Features/RectFeature.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <Graphics/RendererCommandList.h>

namespace Tempest
{
namespace GraphicsFeature
{

void Rects::GatherData(const World& world, FrameData& frameData)
{
	// TODO: Maybe gather data will be better as a system which is executed directly
	flecs::filter filter = flecs::filter(world.m_EntityWorld)
		.include<Components::Transform>()
		.include<Components::Rect>()
		.include_kind(flecs::MatchAll);

	for (flecs::iter itr : world.m_EntityWorld.filter(filter))
	{
		flecs::column transforms = itr.table_column<Components::Transform>();
		flecs::column rects = itr.table_column<Components::Rect>();

		for (int row : itr) {
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
		command.Data = rect;
		commandList.AddCommand(command);
	}
}

GraphicsFeatureDescription Rects::GetDescription()
{
	GraphicsFeatureDescription desc;
	desc.Id = Rects::ID;
	desc.GatherData = &Rects::GatherData;
	desc.GenerateCommands = &Rects::GenerateCommands;
	return desc;
}
}
}