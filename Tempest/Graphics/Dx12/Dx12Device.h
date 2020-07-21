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

	// UI Stuff
	ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
};
}
}