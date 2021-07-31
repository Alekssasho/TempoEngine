#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Dx12
{

struct GraphicsPipelineStateDescription
{
	const void* VSCode = nullptr;
	const void* PSCode = nullptr;
	const void* MSCode = nullptr;
	size_t VSCodeSize = 0;
	size_t PSCodeSize = 0;
	size_t MSCodeSize = 0;
	float DepthBias = 0.0f;
};

class PipelineManager : Utils::NonCopyable
{
public:
	PipelineManager(Dx12Device& device);

	PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineStateDescription& description);

	ID3D12PipelineState* GetPipeline(PipelineStateHandle handle);
	ID3D12RootSignature* GetSignature();
private:
	void PrepareDefaultPipelineStateDesc(struct PipelineStreamBuilder& builder, const GraphicsPipelineStateDescription& description);

	eastl::unordered_map<PipelineStateHandle, ComPtr<ID3D12PipelineState>> m_Pipelines;
	PipelineStateHandle m_NextPipelineHandle = 0;
	Dx12Device& m_Device;
	ComPtr<ID3D12RootSignature> m_Signature;
};
}
}