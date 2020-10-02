#include <Graphics/Managers/PipelineStateManager.h>

namespace Tempest
{
PipelineStateManager::PipelineStateManager(class Renderer& renderer)
	: m_Renderer(renderer)
{
}

PipelineStateHandle PipelineStateManager::RequestPipelineState(const PipelineStateDescription& description)
{
	// TODO: Implement me
	return {};
}

Backend::PipelineHandle PipelineStateManager::GetBackendPipeline(PipelineStateHandle handle)
{
	auto findItr = m_Pipelines.find(handle);
	if (findItr != m_Pipelines.end())
	{
		return findItr->second;
	}

	return 0;
}
}