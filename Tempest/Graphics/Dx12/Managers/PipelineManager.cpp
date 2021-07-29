#include <CommonIncludes.h>

#include <Graphics/Dx12/Managers/PipelineManager.h>

namespace Tempest
{
namespace Dx12
{

template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _Type, typename DataType>
struct alignas(void*) PipelineStreamSubobject
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type;
	DataType Data;

	PipelineStreamSubobject(const DataType& input) : Type(_Type), Data(input) {}
};

struct PipelineStreamBuilder
{
	template<typename SubobjectType>
	void Add(const SubobjectType& type)
	{
		const size_t offset = size_t(Memory.size());
		Memory.resize(Memory.size() + sizeof(SubobjectType));
		memcpy(Memory.data() + offset, &type, sizeof(SubobjectType));
	}

	eastl::vector<uint8_t> Memory;
};

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

	// TODO: This must be in sync with ShaderParameterType
	D3D12_ROOT_PARAMETER params[] = {
		sceneConstants,
		geometryConstants
	};

	D3D12_STATIC_SAMPLER_DESC sampler;
	::ZeroMemory(&sampler, sizeof(D3D12_STATIC_SAMPLER_DESC));
	sampler.Filter  = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	// TODO: We don't need input assembler as we are doing bindless
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
	rootSignatureDesc.NumParameters = UINT(std::size(params));
	rootSignatureDesc.pParameters = params;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	CHECK_SUCCESS(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	CHECK_SUCCESS(m_Device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_Signature)));
}

void PipelineManager::PrepareDefaultPipelineStateDesc(PipelineStreamBuilder& builder)
{
	D3D12_RASTERIZER_DESC rasterizerState = {0};
	rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerState.FrontCounterClockwise = TRUE;
	rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerState.DepthClipEnable = TRUE;
	rasterizerState.MultisampleEnable = FALSE;
	rasterizerState.AntialiasedLineEnable = FALSE;
	rasterizerState.ForcedSampleCount = 0;
	rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_BLEND_DESC blendState = {0};
	blendState.AlphaToCoverageEnable = FALSE;
	blendState.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE, FALSE,
		D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

	D3D12_DEPTH_STENCIL_DESC1 depthStencilState = { 0 };
	depthStencilState.DepthEnable = TRUE;
	depthStencilState.StencilEnable = FALSE;
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	D3D12_RT_FORMAT_ARRAY rtvFormats = { 0 };
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvFormats.NumRenderTargets = 1;

	DXGI_SAMPLE_DESC sampleDesc = {0};
	sampleDesc.Count = 1;

	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>(rasterizerState));
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>(blendState));
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1>(depthStencilState));
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>(DXGI_FORMAT_D24_UNORM_S8_UINT));
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, UINT>(UINT_MAX));
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>(rtvFormats));
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>(sampleDesc));
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE));
}

PipelineStateHandle PipelineManager::CreateGraphicsPipeline(const GraphicsPipelineStateDescription& description)
{
	// TODO: Temp memory
	PipelineStreamBuilder builder;
	builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>(m_Signature.Get()));
	if(description.PSCode)
	{
		builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>(D3D12_SHADER_BYTECODE{
			description.PSCode,
			description.PSCodeSize
		}));
	}

	if(description.MSCode)
	{
		builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE>(D3D12_SHADER_BYTECODE{
			description.MSCode,
			description.MSCodeSize
		}));
	}
	else if(description.VSCode)
	{
		builder.Add(PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>(D3D12_SHADER_BYTECODE{
			description.VSCode,
			description.VSCodeSize
		}));
	}
	else
	{
		assert(false);
	}
	PrepareDefaultPipelineStateDesc(builder);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {0};
	streamDesc.SizeInBytes = builder.Memory.size();
	streamDesc.pPipelineStateSubobjectStream = builder.Memory.data();

	PipelineStateHandle resultHandle = ++m_NextPipelineHandle;
	ComPtr<ID3D12PipelineState>& pipeline = m_Pipelines[resultHandle];
	CHECK_SUCCESS(m_Device.GetDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline)));
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