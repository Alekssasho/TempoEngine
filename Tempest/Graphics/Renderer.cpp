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
	, Managers(*this)
{
	m_RenderFeatures.emplace_back(new GraphicsFeature::Rects);
	m_RenderFeatures.emplace_back(new GraphicsFeature::StaticMesh);
}

Renderer::~Renderer()
{
}

void Renderer::InitializeFeatures(const World& world)
{
	OPTICK_EVENT();
	for (const auto& feature : m_RenderFeatures)
	{
		feature->Initialize(world, *this);
	}
}

bool Renderer::CreateWindowSurface(WindowHandle handle)
{
	m_Backend->Initialize(handle);
	return false;
}

FrameData Renderer::GatherWorldData(const World& world)
{
	OPTICK_EVENT();
	// TODO: No need to return it.
	FrameData frameData;
	for (const auto& feature : m_RenderFeatures)
	{
		feature->GatherData(world, frameData);
	}
	return frameData;
}

void Renderer::RenderFrame(const FrameData& data)
{
	OPTICK_EVENT();

	RendererCommandList commandList;
	for (const auto& feature : m_RenderFeatures)
	{
		feature->GenerateCommands(data, commandList);
	}

	m_Backend->RenderFrame(commandList);
}

RenderManagers::RenderManagers(Renderer& renderer)
	: PipelineState(renderer)
{
}
}

