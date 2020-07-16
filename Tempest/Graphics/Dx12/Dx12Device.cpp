#include <Graphics/Dx12/Dx12Device.h>

namespace Tempest
{
namespace Dx12
{
Dx12Device::Dx12Device()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	CHECK_SUCCESS(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)));

	ComPtr<IDXGIAdapter1> adapter;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_Factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device))))
		{
			break;
		}
	}

	if (!m_Device)
	{
		LOG(Fatal, Dx12, "No valid adapter for creating dx12 device");
		return;
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	CHECK_SUCCESS(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_GraphicsQueue)));
	m_GraphicsQueue->SetName(L"Main Graphics Queue");
}

Dx12Device::~Dx12Device()
{
}

void Dx12Device::Initialize(WindowHandle handle)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	CHECK_SUCCESS(m_Factory->CreateSwapChainForHwnd(
		m_GraphicsQueue.Get(),
		HWND(handle),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	swapChain.As(&m_SwapChain);

	CHECK_SUCCESS(m_Factory->MakeWindowAssociation(HWND(handle), DXGI_MWA_NO_ALT_ENTER));

	{
		DXGI_SWAP_CHAIN_DESC1 realDesc;
		m_SwapChain->GetDesc1(&realDesc);
		m_SwapChainSize = glm::uvec2(realDesc.Width, realDesc.Height);
	}

	FORMAT_LOG(Info, Dx12, "Initialized Swap Chain with size %d %d", m_SwapChainSize.x, m_SwapChainSize.y);

	UINT rtvDescriptorSize;
	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 2;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_SUCCESS(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));

		rtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_SUCCESS(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));
	}

	// Create frame resources.
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < 2; n++)
		{
			m_SwapChainImages.push_back(SwapChainImage{});
			CHECK_SUCCESS(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_SwapChainImages[n].Resource)));
			m_Device->CreateRenderTargetView(m_SwapChainImages[n].Resource.Get(), nullptr, rtvHandle);
			m_SwapChainImages[n].RTVHandle = rtvHandle;

			rtvHandle.ptr += 1 * rtvDescriptorSize;
		}

		// Create DepthStencil texture
		{
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask = 0;
			heapProperties.VisibleNodeMask = 0;

			D3D12_RESOURCE_DESC desc;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Alignment = 0;
			desc.Width = m_SwapChainSize.x;
			desc.Height = m_SwapChainSize.y;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_CLEAR_VALUE clearValue;
			clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			clearValue.DepthStencil.Depth = 1.0f;
			clearValue.DepthStencil.Stencil = 0;

			CHECK_SUCCESS(m_Device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clearValue,
				IID_PPV_ARGS(&m_DSV)
			));


			m_DSVHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
			m_Device->CreateDepthStencilView(m_DSV.Get(), nullptr, m_DSVHandle);
		}
	}

	// Create synchronization objects.
	{
		m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
		m_FenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	// Create Command Lists for number of back buffers
	for (int i = 0; i < m_SwapChainImages.size(); ++i)
	{
		ComPtr<ID3D12CommandAllocator> allocator;
		CHECK_SUCCESS(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
		allocator->SetName(L"Backbuffer Command Allocator");

		ComPtr<ID3D12GraphicsCommandList> list;
		CHECK_SUCCESS(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
		list->SetName(L"Backbuffer Command List");

		list->Close();

		m_MainCommandLists.push_back({ allocator, list });
	}
}

Dx12FrameData Dx12Device::StartNewFrame()
{
	uint32_t index = m_SwapChain->GetCurrentBackBufferIndex();
	CommandList& list = m_MainCommandLists[index];
	list.CommandAllocator->Reset();
	list.DxCommandList->Reset(list.CommandAllocator.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_SwapChainImages[index].Resource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	list.DxCommandList->ResourceBarrier(1, &barrier);

	return Dx12FrameData{
		list.DxCommandList.Get(),
		m_SwapChainImages[index].Resource.Get(),
		m_SwapChainImages[index].RTVHandle,
		m_DSVHandle
	};
}

void Dx12Device::SubmitFrame(const Dx12FrameData& frame)
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = frame.BackBufferResource;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	frame.CommandList->ResourceBarrier(1, &barrier);

	frame.CommandList->Close();

	ID3D12CommandList* cmdList = frame.CommandList;
	m_GraphicsQueue->ExecuteCommandLists(1, &cmdList);
}

void Dx12Device::Present()
{
	m_SwapChain->Present(1, 0);

	const UINT64 fence = m_FenceValue;
	m_GraphicsQueue->Signal(m_Fence.Get(), fence);
	m_FenceValue++;

	// Wait until the previous frame is finished.
	if (m_Fence->GetCompletedValue() < m_FenceValue - 2)
	{
		m_Fence->SetEventOnCompletion(m_FenceValue - 2, m_FenceEvent);
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}
}
}
}