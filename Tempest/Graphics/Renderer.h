#pragma once

#include <Defines.h>
#include <EASTL/unique_ptr.h>

#include <Platform/WindowsPlatform.h>

#include <Graphics/Managers/PipelineStateManager.h>

namespace Tempest
{
class World;
struct RenderFeature;
// This is forward declare and used through a pointer to avoid pulling Dx12 headers into rest of the engine
namespace Dx12 { class Backend; }

struct RenderManagers
{
	RenderManagers(class Renderer& renderer);
	PipelineStateManager PipelineState;
};

class Renderer
{
public:
	Renderer();
	~Renderer();
	bool CreateWindowSurface(WindowHandle handle);

	void InitializeFeatures(const World& world);
	FrameData GatherWorldData(const World& world);
	void RenderFrame(const FrameData& data);

	void RegisterView();

	RenderManagers Managers;

private:
	eastl::unique_ptr<class Dx12::Backend> m_Backend;
	eastl::vector<eastl::unique_ptr<RenderFeature>> m_RenderFeatures;
};
}

