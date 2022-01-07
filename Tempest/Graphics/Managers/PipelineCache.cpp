#include <CommonIncludes.h>

#include <Graphics/Managers/PipelineCache.h>
#include <Graphics/Renderer.h>

namespace Tempest
{

	PipelineCacheManager::PipelineCacheManager(Renderer& renderer)
	: m_Renderer(renderer)
{
}

PipelineStateHandle PipelineCacheManager::GetHandle(const PipelineStateDescription& description)
{
	auto findIt = eastl::find_if(m_Cache.begin(), m_Cache.end(), [&description](auto cachedValue) {
		return cachedValue.first == description;
	});

	if (findIt != m_Cache.end()) {
		return findIt->second;
	}

	auto handle = m_Renderer.RequestPipelineState(description);
	m_Cache.push_back(eastl::make_pair(description, handle));
	return handle;
}
}