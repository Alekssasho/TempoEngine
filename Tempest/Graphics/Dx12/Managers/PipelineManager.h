#pragma once

#include <EASTL/unordered_map.h>
#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Dx12
{

struct GraphicsPipelineStateDescription
{
	const void* VSCode;
	const void* PSCode;
	size_t VSCodeSize;
	size_t PSCodeSize;
};

class PipelineManager
{
public:
	PipelineManager(Dx12Device& device);

	PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineStateDescription& description);

	ID3D12PipelineState* GetPipeline(PipelineStateHandle handle);
	ID3D12RootSignature* GetSignature();
private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PrepareDefaultPipelineStateDesc();

	eastl::unordered_map<PipelineStateHandle, ComPtr<ID3D12PipelineState>> m_Pipelines;
	PipelineStateHandle m_NextPipelineHandle = 0;
	Dx12Device& m_Device;
	ComPtr<ID3D12RootSignature> m_Signature;
};
}
}