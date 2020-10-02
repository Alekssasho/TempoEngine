#pragma once

#include <EASTL/unordered_map.h>
#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/BackendTypes.h>

namespace Tempest
{
namespace Dx12
{

class PipelineManager
{
public:
	PipelineManager(Dx12Device& device);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PrepareDefaultPipelineStateDesc();

	Tempest::Backend::PipelineHandle CreateGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& decs);

	ID3D12PipelineState* GetPipeline(Tempest::Backend::PipelineHandle handle);
private:
	eastl::unordered_map<Tempest::Backend::PipelineHandle, ComPtr<ID3D12PipelineState>> m_Pipelines;
	Tempest::Backend::PipelineHandle m_NextPipelineHandle = 0;
	Dx12Device& m_Device;
};
}
}