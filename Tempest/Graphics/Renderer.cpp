#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <optick.h>

#include <Graphics/RendererCommandList.h>

// TODO: Find a better way than just to include them
#include <Graphics/Features/RectFeature.h>
#include <Graphics/Features/StaticMeshFeature.h>

namespace Tempest
{
Renderer::Renderer()
	: m_Backend(new Dx12::Backend)
{
	RegisterFeature(GraphicsFeature::Rects::GetDescription());
	RegisterFeature(GraphicsFeature::StaticMesh::GetDescription());
}

Renderer::~Renderer()
{
}

bool Renderer::CreateWindowSurface(WindowHandle handle)
{
	m_Backend->Initialize(handle);
	return false;
}

void Renderer::RegisterFeature(const GraphicsFeatureDescription& description)
{
	// TODO: Find a better way to store this
	m_FeatureDescriptions.push_back(description);
}

FrameData Renderer::GatherWorldData(const World& world)
{
	OPTICK_EVENT("Gather World Data");
	// TODO: No need to return it.
	FrameData frameData;
	for (const auto& feature : m_FeatureDescriptions)
	{
		feature.GatherData(world, frameData);
	}
	return frameData;
}

void Renderer::RenderFrame(const FrameData& data)
{
	OPTICK_EVENT("Render Frame");

	RendererCommandList commandList;
	for (const auto& feature : m_FeatureDescriptions)
	{
		feature.GenerateCommands(data, commandList);
	}

	m_Backend->RenderFrame(commandList);
}
}

