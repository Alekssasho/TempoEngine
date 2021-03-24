#include <CommonIncludes.h>

#include <Graphics/Dx12/Managers/PipelineManager.h>

namespace Tempest
{
namespace Dx12
{
PipelineManager::PipelineManager(Dx12Device& device)
	: m_Device(device)
{
	// Scene Constant buffer
	D3D12_ROOT_PARAMETER sceneConstants;
	sceneConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	sceneConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	sceneConstants.Descriptor.ShaderRegister = 0;
	sceneConstants.Descriptor.RegisterSpace = 0;

	// Geometry constant buffer
	D3D12_ROOT_PARAMETER geometryConstants;
	geometryConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	geometryConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	geometryConstants.Descriptor.ShaderRegister = 0;
	geometryConstants.Descriptor.RegisterSpace = 1;

	D3D12_DESCRIPTOR_RANGE geometryDescritorRange;
	geometryDescritorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	geometryDescritorRange.NumDescriptors = UINT_MAX; // Unbounded
	geometryDescritorRange.BaseShaderRegister = 0;
	geometryDescritorRange.RegisterSpace = 1;
	geometryDescritorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	
	D3D12_ROOT_PARAMETER geometryDescriptorTable;
	geometryDescriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	geometryDescriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	geometryDescriptorTable.DescriptorTable.NumDescriptorRanges = 1;
	geometryDescriptorTable.DescriptorTable.pDescriptorRanges = &geometryDescritorRange;

	D3D12_ROOT_PARAMETER params[] = {
		sceneConstants,
		geometryConstants,
		geometryDescriptorTable
	};

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = UINT(std::size(params));
	rootSignatureDesc.pParameters = params;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	CHECK_SUCCESS(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	CHECK_SUCCESS(m_Device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_Signature)));
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

	desc.DepthStencilState.DepthEnable = TRUE;
	desc.DepthStencilState.StencilEnable = FALSE;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.SampleDesc.Count = 1;

	return desc;
}

PipelineStateHandle PipelineManager::CreateGraphicsPipeline(const GraphicsPipelineStateDescription& description)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC dx12Desc = PrepareDefaultPipelineStateDesc();
	dx12Desc.VS.pShaderBytecode = description.VSCode;
	dx12Desc.VS.BytecodeLength = description.VSCodeSize;
	dx12Desc.PS.pShaderBytecode = description.PSCode;
	dx12Desc.PS.BytecodeLength = description.PSCodeSize;

	dx12Desc.pRootSignature = m_Signature.Get();

	PipelineStateHandle resultHandle = ++m_NextPipelineHandle;
	ComPtr<ID3D12PipelineState>& pipeline = m_Pipelines[resultHandle];
	CHECK_SUCCESS(m_Device.GetDevice()->CreateGraphicsPipelineState(&dx12Desc, IID_PPV_ARGS(&pipeline)));
	return resultHandle;
}

ID3D12PipelineState* PipelineManager::GetPipeline(PipelineStateHandle handle)
{
	return m_Pipelines[handle].Get();
}

ID3D12RootSignature* PipelineManager::GetSignature()
{
	return m_Signature.Get();
}
}
}