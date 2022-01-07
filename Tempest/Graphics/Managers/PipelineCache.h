#pragma once

#include <Graphics/RendererTypes.h>

namespace Tempest
{
class PipelineCacheManager
{
public:
	PipelineCacheManager(class Renderer& renderer);

	PipelineStateHandle GetHandle(const PipelineStateDescription& description);

private:
	Renderer& m_Renderer;
	eastl::vector<eastl::pair<PipelineStateDescription, PipelineStateHandle>> m_Cache;
};
}