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

	const uint8_t* commandListIterator = commandList.m_DataBuffer.begin();
	while (commandListIterator && commandListIterator < commandList.m_DataBuffer.end())
	{
		// TODO: make this better
		RendererCommandType type = reinterpret_cast<const RendererCommand<RendererCommandType::Count>*>(commandListIterator)->Type;
		switch (type)
		{
		case RendererCommandType::DrawRect:
		{
			const RendererCommandDrawRect* command = reinterpret_cast<const RendererCommandDrawRect*>(commandListIterator);
			setPipeline(command->Pipeline);
			frame.CommandList->SetGraphicsRoot32BitConstants(0, 8, &command->Data, 0);
			frame.CommandList->DrawInstanced(4, 1, 0, 0);

			commandListIterator += sizeof(RendererCommandDrawRect);
			break;
		}
		case RendererCommandType::DrawInstanced:
		{
			const RendererCommandDrawInstanced* command = reinterpret_cast<const RendererCommandDrawInstanced*>(commandListIterator);
			setPipeline(command->Pipeline);
			frame.CommandList->DrawInstanced(command->VertexCountPerInstance, command->InstanceCount, 0, 0);
			//frame.CommandList->SetGraphicsRoot32BitConstants(0, 8, &command->Data, 0);

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