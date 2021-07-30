#include <CommonIncludes.h>

#include <Graphics/Dx12/Dx12Backend.h>
#include <Graphics/RendererCommandList.h>
#include <Graphics/Renderer.h>
#include <Graphics/FrameData.h>
#include <EngineCore.h>

#ifdef _DEBUG
#include <initguid.h>
#include <dxgidebug.h>
#endif

#include <eastl/optional.h>

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

void Backend::Initialize(WindowHandle handle)
{
	m_Device->Initialize(handle);
}

static D3D12_RESOURCE_STATES Dx12StateFromState(ResourceState state)
{
	switch (state)
	{
	case ResourceState::PixelShaderRead: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case ResourceState::DepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case ResourceState::DepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
	default:
		assert(false);
		return D3D12_RESOURCE_STATE_COMMON;
	}
}

void Backend::RenderFrame(const RendererCommandList& commandList)
{
	Dx12::Dx12FrameData frame = m_Device->StartNewFrame();

	// TODO: This should probably be part of the pipeline itself
	frame.CommandList->SetGraphicsRootSignature(Managers.Pipeline.GetSignature());
	// TODO: better support for indices of root parameters
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

	const D3D12_GPU_VIRTUAL_ADDRESS constantBufferData = m_Device->GetConstantDataManager().GetGPUAddress();
	auto setShaderParameters = [&](const ShaderParameterView parameters[size_t(ShaderParameterType::Count)]) {
		// TODO: Maybe some kind of cache ?
		for(int i = 0; i < int(ShaderParameterType::Count); ++i)
		{
			frame.CommandList->SetGraphicsRootConstantBufferView(i, constantBufferData + parameters[i].ConstantDataOffset);
		}
	};

	const uint8_t* commandListIterator = commandList.m_DataBuffer.begin();
	while (commandListIterator && commandListIterator < commandList.m_DataBuffer.end())
	{
		// TODO: make this better
		RendererCommandType type = reinterpret_cast<const RendererCommand<RendererCommandType::Count>*>(commandListIterator)->Type;
		switch (type)
		{
		// TODO: Maybe use render pass api
		case RendererCommandType::BeginRenderPass:
		{
			const RendererCommandBeginRenderPass* command = reinterpret_cast<const RendererCommandBeginRenderPass*>(commandListIterator);

			eastl::optional<D3D12_CPU_DESCRIPTOR_HANDLE> rtv;
			eastl::optional<D3D12_CPU_DESCRIPTOR_HANDLE> dsv;
			uint32_t width = 0;
			uint32_t height = 0;
			if(command->ColorTarget.Texture == -2)
			{
				rtv = frame.BackBufferRTV;
				width = m_Device->GetSwapChainSize().x;
				height = m_Device->GetSwapChainSize().y;
			}

			if(command->DepthStencilTarget.Texture == -2)
			{
				dsv = frame.BackBufferDSV;
				width = m_Device->GetSwapChainSize().x;
				height = m_Device->GetSwapChainSize().y;
			}
			else if (command->DepthStencilTarget.Texture != sInvalidHandle)
			{
				uint32_t slot = GetDevice()->m_DSVDescriptorHeap.AllocateDynamicResource();

				D3D12_CPU_DESCRIPTOR_HANDLE heapStart = GetDevice()->m_DSVDescriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart();
				UINT heapIncrement = GetDevice()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
				heapStart.ptr += heapIncrement * slot;
				GetDevice()->GetDevice()->CreateDepthStencilView(Managers.Texture.GetTexture(command->DepthStencilTarget.Texture), nullptr, heapStart);
				dsv = heapStart;

				auto dimensions = Managers.Texture.GetTextureDimensions(command->DepthStencilTarget.Texture);
				width = dimensions.x;
				height = dimensions.y;
			}

			if(rtv && command->ColorTarget.LoadAction == TextureTargetLoadAction::Clear)
			{
				const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
				frame.CommandList->ClearRenderTargetView(*rtv, clearColor, 0, nullptr);
			}
			if(dsv && command->DepthStencilTarget.LoadAction == TextureTargetLoadAction::Clear)
			{
				frame.CommandList->ClearDepthStencilView(*dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			}

			frame.CommandList->OMSetRenderTargets(rtv ? 1 : 0, rtv ? &*rtv : nullptr, 0, dsv ? &*dsv : nullptr);
			// TODO: This should be taken from somewhere
			D3D12_VIEWPORT viewport;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = float(width);
			viewport.Height = float(height);
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1;
			frame.CommandList->RSSetViewports(1, &viewport);

			D3D12_RECT scissor;
			scissor.left = 0;
			scissor.top = 0;
			scissor.right = width;
			scissor.bottom = height;
			frame.CommandList->RSSetScissorRects(1, &scissor);

			commandListIterator += sizeof(RendererCommandBeginRenderPass);
			break;
		}
		case RendererCommandType::EndRenderPass:
		{
			commandListIterator += sizeof(RendererCommandEndRenderPass);
			break;
		}
		case RendererCommandType::Barrier:
		{
			const RendererCommandBarrier* command = reinterpret_cast<const RendererCommandBarrier*>(commandListIterator);

			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = Managers.Texture.GetTexture(command->TextureHandle); // TODO: It could be a buffer ?
			barrier.Transition.StateBefore = Dx12StateFromState(command->BeforeState);
			barrier.Transition.StateAfter = Dx12StateFromState(command->AfterState);
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			frame.CommandList->ResourceBarrier(1, &barrier);

			commandListIterator += sizeof(RendererCommandBarrier);
			break;
		}
		case RendererCommandType::DrawInstanced:
		{
			const RendererCommandDrawInstanced* command = reinterpret_cast<const RendererCommandDrawInstanced*>(commandListIterator);
			setPipeline(command->Pipeline);
			setShaderParameters(command->ParameterViews);
			frame.CommandList->DrawInstanced(command->VertexCountPerInstance, command->InstanceCount, 0, 0);

			commandListIterator += sizeof(RendererCommandDrawInstanced);
			break;
		}
		case RendererCommandType::DrawMeshlet:
		{
			const RendererCommandDrawMeshlet* command = reinterpret_cast<const RendererCommandDrawMeshlet*>(commandListIterator);
			setPipeline(command->Pipeline);
			setShaderParameters(command->ParameterViews);
			frame.CommandList->DispatchMesh(command->MeshletCount, 1, 1);
			commandListIterator += sizeof(RendererCommandDrawMeshlet);
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

UploadData Backend::PrepareUpload(uint32_t size)
{
	UploadData result;

	D3D12_HEAP_PROPERTIES props;
	::ZeroMemory(&props, sizeof(D3D12_HEAP_PROPERTIES));
	props.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CHECK_SUCCESS(m_Device->GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&result.UploadHeap)));
	
	D3D12_RANGE range;
	::ZeroMemory(&range, sizeof(D3D12_RANGE)); // This will tell that we won't read the data from CPU
	CHECK_SUCCESS(result.UploadHeap->Map(0, &range, &result.MappedData));
	result.CurrentOffset = 0;

	// TODO: this could be copy only, but we cannot make any resource barriers on copy lists.
	CHECK_SUCCESS(m_Device->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result.Allocator)));
	result.Allocator->SetName(L"Copy Command Allocator");

	CHECK_SUCCESS(m_Device->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, result.Allocator.Get(), nullptr, IID_PPV_ARGS(&result.CommandList)));
	result.CommandList->SetName(L"Copy Command List");

	return result;
}

void Backend::ExecuteUpload(UploadData& uploadData)
{
	// For good measure
	uploadData.UploadHeap->Unmap(0, nullptr);

	uploadData.CommandList->Close();
	ID3D12CommandList* cmdList = uploadData.CommandList.Get();
	m_Device->m_GraphicsQueue->ExecuteCommandLists(1, &cmdList);
	
	const UINT64 fence = m_Device->m_FenceValue;
	m_Device->m_GraphicsQueue->Signal(m_Device->m_Fence.Get(), fence);
	m_Device->m_FenceValue++;

	if(m_Device->m_Fence->GetCompletedValue() < m_Device->m_FenceValue - 1)
	{
		m_Device->m_Fence->SetEventOnCompletion(m_Device->m_FenceValue - 1, m_Device->m_FenceEvent);
		WaitForSingleObject(m_Device->m_FenceEvent, INFINITE);
	}
}
}
}