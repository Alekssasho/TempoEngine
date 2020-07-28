#include <Graphics/Dx12/Dx12Backend.h>

#ifdef _DEBUG
#include <initguid.h>
#include <dxgidebug.h>
#endif

#include <fstream>
#include <DataDefinitions/ShaderLibrary_generated.h>


namespace Tempest
{
namespace Dx12
{
Backend::Backend()
	: m_Device(new Dx12::Dx12Device)
	, m_PipelineManager(*m_Device)
{
}

Backend::~Backend()
{
	// Reset it by hand to force cleaning of all Dx References and then report live objects to debug
	m_Device.reset();
#ifdef _DEBUG
	{
		ComPtr<IDXGIDebug1> debug;
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	}
#endif
}

void Backend::Initialize(WindowHandle handle)
{
	m_Device->Initialize(handle);

	// Load Shader Library
	std::ifstream stream("../../Tempest/Shaders/ShaderLibrary.tslb", std::ios::binary);
	assert(stream.good());
	stream.seekg(0, std::ios::end);
	size_t size = stream.tellg();
	stream.seekg(0);

	eastl::vector<uint8_t> data(size);
	stream.read((char*)data.data(), size);

	const Definition::ShaderLibrary* shaderLib = Definition::GetShaderLibrary(data.data());
	flatbuffers::Verifier verifier(data.data(), data.size());
	assert(shaderLib->Verify(verifier));

	auto shaders = shaderLib->Shaders();
	eastl::vector<const char*> shaderNames;
	for (auto sh : *shaders) {
		shaderNames.push_back(sh->Name()->c_str());
	}
	auto uiVsShader = shaders->LookupByKey("UI-VS");
	assert(uiVsShader);
	auto uiPsShader = shaders->LookupByKey("UI-PS");
	assert(uiPsShader);
	assert(uiVsShader->Type() == Definition::ShaderType_Vertex);
	assert(uiPsShader->Type() == Definition::ShaderType_Pixel);

	auto desc = m_PipelineManager.PrepareDefaultPipelineStateDesc();
	desc.VS.pShaderBytecode = uiVsShader->Code()->Data();
	desc.VS.BytecodeLength = uiVsShader->Code()->size();
	desc.PS.pShaderBytecode = uiPsShader->Code()->Data();
	desc.PS.BytecodeLength = uiPsShader->Code()->size();

	D3D12_ROOT_PARAMETER rootParam;
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParam.Constants.Num32BitValues = 4;
	rootParam.Constants.RegisterSpace = 0;
	rootParam.Constants.ShaderRegister = 0;

	D3D12_DESCRIPTOR_RANGE range;
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParam2;
	rootParam2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam2.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParam2.DescriptorTable.NumDescriptorRanges = 1;
	rootParam2.DescriptorTable.pDescriptorRanges = &range;

	D3D12_STATIC_SAMPLER_DESC sampler;
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.MinLOD = 0;
	sampler.MaxLOD = 0;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_PARAMETER params[2] = { rootParam, rootParam2 };
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = params;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ComPtr<ID3D12RootSignature> rootSignature;
	CHECK_SUCCESS(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	CHECK_SUCCESS(m_Device->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	desc.pRootSignature = rootSignature.Get();

	eastl::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	inputElements.resize(3);
	inputElements[0].SemanticName = "POSITION";
	inputElements[0].SemanticIndex = 0;
	inputElements[0].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElements[0].InputSlot = 0;
	inputElements[0].AlignedByteOffset = 0;
	inputElements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElements[0].InstanceDataStepRate = 0;

	inputElements[1].SemanticName = "TEXCOORD";
	inputElements[1].SemanticIndex = 0;
	inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElements[1].InputSlot = 0;
	inputElements[1].AlignedByteOffset = 2 * sizeof(float);
	inputElements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElements[1].InstanceDataStepRate = 0;

	inputElements[2].SemanticName = "COLOR";
	inputElements[2].SemanticIndex = 0;
	inputElements[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	inputElements[2].InputSlot = 0;
	inputElements[2].AlignedByteOffset = 4 * sizeof(float);
	inputElements[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElements[2].InstanceDataStepRate = 0;

	desc.InputLayout.pInputElementDescs = inputElements.data();
	desc.InputLayout.NumElements = UINT(inputElements.size());

	m_MeshPipelineHandle = m_PipelineManager.CreateGraphicsPipeline(desc);
}

void Backend::RenderFrame()
{
	Dx12::Dx12FrameData frame = m_Device->StartNewFrame();

	const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	frame.CommandList->ClearRenderTargetView(frame.BackBufferRTV, clearColor, 0, nullptr);

	m_Device->SubmitFrame(frame);

	m_Device->Present();
}
}
}