#include <Graphics/Dx12/Managers/PipelineManager.h>

namespace Tempest
{
namespace Dx12
{
PipelineManager::PipelineManager(Dx12Device& device)
	: m_Device(device)
{
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineManager::PrepareDefaultPipelineStateDesc()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	desc.RasterizerState.FrontCounterClockwise = TRUE;
	desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.RasterizerState.DepthClipEnable = TRUE;
	desc.RasterizerState.MultisampleEnable = FALSE;
	desc.RasterizerState.AntialiasedLineEnable = FALSE;
	desc.RasterizerState.ForcedSampleCount = 0;
	desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	desc.BlendState.AlphaToCoverageEnable = FALSE;
	desc.BlendState.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE, FALSE,
		D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		desc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

	desc.DepthStencilState.DepthEnable = FALSE;
	desc.DepthStencilState.StencilEnable = FALSE;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;

	return desc;
}

Tempest::Backend::PipelineHandle PipelineManager::CreateGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	Tempest::Backend::PipelineHandle resultHandle = m_NextPipelineHandle++;
	ComPtr<ID3D12PipelineState>& pipeline = m_Pipelines[resultHandle];
	CHECK_SUCCESS(m_Device.GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline)));
	return resultHandle;
}

ID3D12PipelineState* PipelineManager::GetPipeline(Tempest::Backend::PipelineHandle handle)
{
	return m_Pipelines[handle].Get();
}
}
}