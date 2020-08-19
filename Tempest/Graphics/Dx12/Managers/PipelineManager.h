#pragma once

#include <EASTL/unordered_map.h>
#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Dx12Device.h>

namespace Tempest
{
namespace Dx12
{
using PipelineHandle = uint32_t;

class PipelineManager
{
public:
	PipelineManager(Dx12Device& device);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PrepareDefaultPipelineStateDesc();

	PipelineHandle CreateGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& decs);

	ID3D12PipelineState* GetPipeline(PipelineHandle handle);
private:
	eastl::unordered_map<PipelineHandle, ComPtr<ID3D12PipelineState>> m_Pipelines;
	PipelineHandle m_NextPipelineHandle = 0;
	Dx12Device& m_Device;
};
}
}