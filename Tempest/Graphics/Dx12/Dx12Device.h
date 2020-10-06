#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Platform/WindowsPlatform.h>
#include <Math/Math.h>
#include <EASTL/vector.h>

namespace Tempest
{
namespace Dx12
{
struct Dx12FrameData
{
	uint32_t BackBufferIndex;
	ID3D12GraphicsCommandList* CommandList;
	ID3D12Resource* BackBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRTV;
	D3D12_CPU_DESCRIPTOR_HANDLE BackBufferDSV;
};

class Dx12Device
{
public:
	Dx12Device();
	~Dx12Device();
	void Initialize(WindowHandle handle);
	Dx12FrameData StartNewFrame();
	void SubmitFrame(const Dx12FrameData& frame);
	void Present();
	glm::uvec2 GetSwapChainSize()
	{
		return m_SwapChainSize;
	}

	ID3D12Device* GetDevice()
	{
		return m_Device.Get();
	}

	void CopyResources(ID3D12Resource* dst, ID3D12Resource* src, D3D12_RESOURCE_STATES requiredDstState);
	// TODO: This should be refactored
	void AddBufferDescriptor(ID3D12Resource* resource, uint32_t numBytes) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHeapStart()
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		return handle;
	}
private:
	ComPtr<IDXGIFactory4> m_Factory;
	ComPtr<ID3D12Device> m_Device;
	ComPtr<ID3D12CommandQueue> m_GraphicsQueue;
	ComPtr<IDXGISwapChain3> m_SwapChain;
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
	ComPtr<ID3D12Resource> m_DSV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;
	struct SwapChainImage
	{
		ComPtr<ID3D12Resource> Resource;
		D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
	};
	eastl::vector<SwapChainImage> m_SwapChainImages;
	glm::uvec2 m_SwapChainSize;

	ComPtr<ID3D12Fence> m_Fence;
	UINT64 m_FenceValue;
	HANDLE m_FenceEvent;

	struct CommandList
	{
		ComPtr<ID3D12CommandAllocator> CommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> DxCommandList;
	};
	eastl::vector<CommandList> m_MainCommandLists;

	CommandList copyList;

	// UI Stuff
	ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
};
}
}