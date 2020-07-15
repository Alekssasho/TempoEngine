#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Platform/WindowsPlatform.h>
#include <Math/Math.h>
#include <EASTL/vector.h>

namespace Tempest
{
namespace Dx12
{
class Dx12Device
{
public:
	Dx12Device();
	void Initialize(WindowHandle handle);
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
};
}
}