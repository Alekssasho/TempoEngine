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
	// TODO: move to Init/deinit methods as using constructor/desctuctor is very rigid and cannot do things properly
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

struct SceneConstantData
{
	glm::mat4x4 ViewProjection;
};

void Backend::Initialize(WindowHandle handle)
{
	m_Device->Initialize(handle);

	// Allocate scene constant buffers
	for (int i = 0; i < std::size(m_SceneConstantBufferData); ++i)
	{
		m_SceneConstantBufferData[i] = Managers.Buffer.CreateBuffer({ BufferType::Constant, sizeof(SceneConstantData), nullptr });
	}
}

void Backend::RenderFrame(const Camera* view, const RendererCommandList& commandList)
{
	Dx12::Dx12FrameData frame = m_Device->StartNewFrame();

	const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	frame.CommandList->ClearRenderTargetView(frame.BackBufferRTV, clearColor, 0, nullptr);
	frame.CommandList->ClearDepthStencilView(frame.BackBufferDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	frame.CommandList->OMSetRenderTargets(1, &frame.BackBufferRTV, 0, &frame.BackBufferDSV);
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
	// TODO: better support for indices of root parameters
	frame.CommandList->SetGraphicsRootDescriptorTable(2, m_Device->GetSRVHeapStart());
	frame.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
	SceneConstantData sceneData{ view->GetViewProjection() };
	Managers.Buffer.MapWriteData(m_SceneConstantBufferData[frame.BackBufferIndex], &sceneData, sizeof(SceneConstantData));

	// TODO: Make this not allocate every frame
	if(commandList.m_ConstantBufferData.size() > 0)
	{
		if(m_GeometryConstantBufferData[frame.BackBufferIndex] != sInvalidHandle)
		{
			Managers.Buffer.DestroyBuffer(m_GeometryConstantBufferData[frame.BackBufferIndex]);
		}

		BufferHandle bufferHandle = Managers.Buffer.CreateBuffer({ BufferType::Constant, commandList.m_ConstantBufferData.size(), nullptr });
		m_GeometryConstantBufferData[frame.BackBufferIndex] = bufferHandle;

		Managers.Buffer.MapWriteData(bufferHandle, commandList.m_ConstantBufferData.data(), commandList.m_ConstantBufferData.size());
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
			frame.CommandList->SetGraphicsRootConstantBufferView(0, Managers.Buffer.GetGPUAddress(m_SceneConstantBufferData[frame.BackBufferIndex]));
			frame.CommandList->SetGraphicsRootConstantBufferView(1, Managers.Buffer.GetGPUAddress(m_GeometryConstantBufferData[frame.BackBufferIndex]) + command->ParameterView.GeometryConstantDataOffset);
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