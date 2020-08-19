#pragma once

#include <Defines.h>
#include <EASTL/unique_ptr.h>

#include <Platform/WindowsPlatform.h>

#include <Graphics/RendererTypes.h>

namespace Tempest
{
class World;
// This is forward declare and used through a pointer to avoid pulling Dx12 headers into rest of the engine
namespace Dx12 { class Backend; }

class Renderer
{
public:
	Renderer();
	~Renderer();
	bool CreateWindowSurface(WindowHandle handle);

	FrameData GatherWorldData(const World& world);
	void RenderFrame(const FrameData& data);

	void RegisterView();
private:
	void RegisterFeature(const GraphicsFeatureDescription& description);

	eastl::unique_ptr<class Dx12::Backend> m_Backend;
	eastl::vector<GraphicsFeatureDescription> m_FeatureDescriptions;
};
}

