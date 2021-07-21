#include <CommonIncludes.h>

#include <Graphics/Dx12/Dx12Device.h>

#include <optick.h>

#include <imgui.h>
// TODO: Reimplement as our own
#include <imgui_impl_dx12.h>

// DirectX 12 Agility SDK exports
extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

namespace Tempest
{
namespace Dx12
{
Dx12Device::Dx12Device()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug1> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(true);

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


	// Check Feature Supports
	D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Options = {0};
	CHECK_SUCCESS(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Options, sizeof(d3d12Options)));

	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{ D3D_SHADER_MODEL_6_6 };
	CHECK_SUCCESS(m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 d3d12OptionsX;
	CHECK_SUCCESS(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &d3d12OptionsX, sizeof(d3d12OptionsX)));
}

Dx12Device::~Dx12Device()
{
	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_Shutdown();
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
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			::ZeroMemory(&rtvDesc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			m_Device->CreateRenderTargetView(m_SwapChainImages[n].Resource.Get(), &rtvDesc, rtvHandle);
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

		ComPtr<ID3D12GraphicsCommandList6> list;
		CHECK_SUCCESS(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
		list->SetName(L"Backbuffer Command List");

		list->Close();

		m_MainCommandLists.push_back({ allocator, list });
	}

	//{
	//	// TODO: this could be copy only, but we cannot make any resource barriers on copy lists.
	//	CHECK_SUCCESS(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&copyList.CommandAllocator)));
	//	copyList.CommandAllocator->SetName(L"Copy Command Allocator");

	//	CHECK_SUCCESS(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, copyList.CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&copyList.DxCommandList)));
	//	copyList.DxCommandList->SetName(L"Copy Command List");

	//	copyList.DxCommandList->Close();
	//}

	// Initialize UI
	{
		auto& io = ImGui::GetIO();
		io.DisplaySize.x = float(m_SwapChainSize.x);
		io.DisplaySize.y = float(m_SwapChainSize.y);
		io.IniFilename = nullptr;

		unsigned char* pixels;
		int w, h;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
		//io.Fonts->TexID = reinterpret_cast<void*>(Modules.Renderer.UITextureLoaded(pixels, w, h));
		ImGui::StyleColorsDark();

		// TODO: Make this real code and remove imgui_impl_dx12

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		CHECK_SUCCESS(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_UISRVHeap)));

		ImGui_ImplDX12_Init(
			m_Device.Get(),
			2,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			m_UISRVHeap.Get(),
			m_UISRVHeap->GetCPUDescriptorHandleForHeapStart(),
			m_UISRVHeap->GetGPUDescriptorHandleForHeapStart()
		);

		ImGui_ImplDX12_CreateDeviceObjects();
	}
}

Dx12FrameData Dx12Device::StartNewFrame()
{
	uint32_t index = m_SwapChain->GetCurrentBackBufferIndex();
	CommandList& list = m_MainCommandLists[index];
	list.CommandAllocator->Reset();
	list.DxCommandList->Reset(list.CommandAllocator.Get(), nullptr);

	assert(m_MainDescriptorHeap.Heap);
	list.DxCommandList->SetDescriptorHeaps(1, m_MainDescriptorHeap.Heap.GetAddressOf());

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_SwapChainImages[index].Resource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	list.DxCommandList->ResourceBarrier(1, &barrier);

	return Dx12FrameData{
		index,
		list.DxCommandList.Get(),
		m_SwapChainImages[index].Resource.Get(),
		m_SwapChainImages[index].RTVHandle,
		m_DSVHandle
	};
}

void Dx12Device::SubmitFrame(const Dx12FrameData& frame)
{
	// UI Rendering before we finish the frame
	{
		OPTICK_EVENT("ImGUI CPU Render");
		ImGui::Render();
		ImGui_ImplDX12_NewFrame();
	}
	{
		// TODO: Merge ui srv heap into main descriptor heap
		OPTICK_EVENT("ImGUI GPU Draw");
		frame.CommandList->OMSetRenderTargets(1, &frame.BackBufferRTV, false, nullptr);
		frame.CommandList->SetDescriptorHeaps(1, m_UISRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), frame.CommandList);
	}

	// Finish the frame itself
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

void Dx12Device::AllocateMainDescriptorHeap(const int numTextures)
{
	m_MainDescriptorHeap.Initialize(m_Device.Get());
	m_MainDescriptorHeap.AllocateStaticResources(static_cast<int>(ShaderResourceSlot::NonTextureCount) + numTextures);
}

void Dx12Device::AddStaticTextureDescriptor(ID3D12Resource* resource, DXGI_FORMAT format, uint32_t mipLevels, uint32_t slot)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
	desc.Format = format;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.MipLevels = mipLevels;
	desc.Texture2D.PlaneSlice = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;

	// Go to appropriate descriptor
	D3D12_CPU_DESCRIPTOR_HANDLE handle(m_MainDescriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart());
	handle.ptr += (slot + static_cast<uint32_t>(ShaderResourceSlot::TextureStart)) * m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_Device->CreateShaderResourceView(
		resource,
		&desc,
		handle
	);
}

void Dx12Device::AddStaticBufferDescriptor(ID3D12Resource* resource, uint32_t numElements, uint32_t stride, ShaderResourceSlot slot) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
	// Format_unknown is for StructuredBuffers
	// r32_typeless is for ByteAddressBuffer + SRV_FLAG_RAW
	desc.Format = slot == ShaderResourceSlot::MeshletIndices ? DXGI_FORMAT_R8_UINT : DXGI_FORMAT_UNKNOWN; //DXGI_FORMAT_R32_TYPELESS DXGI_FORMAT_UNKNOWN
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Buffer.FirstElement = 0;
	desc.Buffer.NumElements = numElements;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	if(desc.Format == DXGI_FORMAT_UNKNOWN)
	{
		desc.Buffer.StructureByteStride = stride;
	}

	// Go to appropriate descriptor
	D3D12_CPU_DESCRIPTOR_HANDLE handle(m_MainDescriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart());
	handle.ptr += static_cast<uint32_t>(slot) * m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_Device->CreateShaderResourceView(
		resource,
		&desc,
		handle
	);
}

}
}