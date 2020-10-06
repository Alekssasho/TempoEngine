#include <Graphics/Dx12/Dx12Backend.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/Renderer.h>
#include <EngineCore.h>

#ifdef _DEBUG
#include <initguid.h>
#include <dxgidebug.h>
#endif

namespace Tempest
{
namespace Dx12
{
Backend::Backend()
	: m_Device(new Dx12::Dx12Device)
	, Managers(*m_Device)
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
}

void Backend::RenderFrame(const RendererCommandList& commandList)
{
	Dx12::Dx12FrameData frame = m_Device->StartNewFrame();

	const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	frame.CommandList->ClearRenderTargetView(frame.BackBufferRTV, clearColor, 0, nullptr);

	// Draw Rects
	frame.CommandList->OMSetRenderTargets(1, &frame.BackBufferRTV, 0, nullptr);
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = float(m_Device->GetSwapChainSize().x);
	viewport.Height = float(m_Device->GetSwapChainSize().y);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	frame.CommandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissor;
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = m_Device->GetSwapChainSize().x;
	scissor.bottom = m_Device->GetSwapChainSize().y;
	frame.CommandList->RSSetScissorRects(1, &scissor);

	// TODO: This should probably be part of the pipeline itself
	frame.CommandList->SetGraphicsRootSignature(Managers.Pipeline.GetSignature());
	frame.CommandList->SetGraphicsRootDescriptorTable(1, m_Device->GetSRVHeapStart());
	frame.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	PipelineStateHandle currentPipeline = sInvalidHandle;
	auto setPipeline = [&](PipelineStateHandle requestedHandle) {
		if (currentPipeline != requestedHandle)
		{
			currentPipeline = requestedHandle;
			frame.CommandList->SetPipelineState(
				Managers.Pipeline.GetPipeline(requestedHandle)
			);
		}
	};

	// Prepare constant buffer data
	// TODO: Make this not allocate every frame
	if(commandList.m_ConstantBufferData.size() > 0)
	{
		m_ConstantBufferData[frame.BackBufferIndex].Reset();

		D3D12_HEAP_PROPERTIES props;
		::ZeroMemory(&props, sizeof(D3D12_HEAP_PROPERTIES));
		props.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC desc;
		::ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = commandList.m_ConstantBufferData.size();
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		CHECK_SUCCESS(m_Device->GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_ConstantBufferData[frame.BackBufferIndex])));

		D3D12_RANGE range;
		::ZeroMemory(&range, sizeof(D3D12_RANGE)); // This will tell that we won't read the data from CPU
		void* dataPointer = nullptr;
		CHECK_SUCCESS(m_ConstantBufferData[frame.BackBufferIndex]->Map(0, &range, &dataPointer));

		memcpy(dataPointer, commandList.m_ConstantBufferData.data(), commandList.m_ConstantBufferData.size());

		m_ConstantBufferData[frame.BackBufferIndex]->Unmap(0, nullptr);
	}

	const uint8_t* commandListIterator = commandList.m_DataBuffer.begin();
	while (commandListIterator && commandListIterator < commandList.m_DataBuffer.end())
	{
		// TODO: make this better
		RendererCommandType type = reinterpret_cast<const RendererCommand<RendererCommandType::Count>*>(commandListIterator)->Type;
		switch (type)
		{
		case RendererCommandType::DrawInstanced:
		{
			const RendererCommandDrawInstanced* command = reinterpret_cast<const RendererCommandDrawInstanced*>(commandListIterator);
			setPipeline(command->Pipeline);
			frame.CommandList->SetGraphicsRootConstantBufferView(0, m_ConstantBufferData[frame.BackBufferIndex]->GetGPUVirtualAddress() + command->ParameterView.GeometryConstantDataOffset);
			frame.CommandList->DrawInstanced(command->VertexCountPerInstance, command->InstanceCount, 0, 0);

			commandListIterator += sizeof(RendererCommandDrawInstanced);
			break;
		}
		default:
			assert(false);
			break;
		}
	}

	m_Device->SubmitFrame(frame);

	m_Device->Present();
}
}
}