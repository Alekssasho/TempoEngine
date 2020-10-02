#pragma once

#include <EASTL/unordered_map.h>
#include <Graphics/BackendTypes.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
struct PipelineStateDescription
{
	const char* ShaderName;
};

class PipelineStateManager
{
public:
	PipelineStateManager(class Renderer& renderer);

	PipelineStateHandle RequestPipelineState(const PipelineStateDescription& description);
	Backend::PipelineHandle GetBackendPipeline(PipelineStateHandle handle);
private:
	Renderer& m_Renderer;
	eastl::unordered_map<PipelineStateHandle, Backend::PipelineHandle> m_Pipelines;
};
}