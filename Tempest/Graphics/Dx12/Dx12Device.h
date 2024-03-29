#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Managers/TwoPartRingBufferDescriptorHeapManager.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>
#include <Graphics/Dx12/Managers/TextureManager.h>
#include <Platform/WindowsPlatform.h>

namespace Tempest
{
namespace Dx12
{
struct Dx12FrameData
{
	uint32_t BackBufferIndex;
	ID3D12GraphicsCommandList6* CommandList;
	ID3D12Resource* BackBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRTV;
	D3D12_CPU_DESCRIPTOR_HANDLE BackBufferDSV;
};

class Dx12Device : Utils::NonCopyable
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

	ID3D12Device3* GetDevice()
	{
		return m_Device.Get();
	}

	ConstantBufferDataManager& GetConstantDataManager()
	{
		return m_ConstantBufferData;
	}

	void AllocateMainDescriptorHeap(const int numTextures);

	// TODO: This should be refactored
	enum class ShaderResourceSlot
	{
		//UI, This is in another heap
		Meshlets,
		MeshletIndices,
		MeshletVertices,
		Materials,
		TextureStart,
		NonTextureCount = TextureStart
	};
	void AddStaticBufferDescriptor(ID3D12Resource* resource, uint32_t numElements, uint32_t stride, ShaderResourceSlot slot) const;
	void AddStaticTextureDescriptor(ID3D12Resource* resource, DXGI_FORMAT format, uint32_t mipLevels, uint32_t slot);
	// TODO: Remove this abstraction and just use device code inside the backend
public:
	ComPtr<IDXGIFactory4> m_Factory;
	ComPtr<ID3D12Device3> m_Device;
	ComPtr<ID3D12CommandQueue> m_GraphicsQueue;
	ComPtr<IDXGISwapChain3> m_SwapChain;
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
		ComPtr<ID3D12GraphicsCommandList6> DxCommandList;
	};
	eastl::vector<CommandList> m_MainCommandLists;

	TwoPartRingBufferDescriptorHeapManager m_MainDescriptorHeap;
	TwoPartRingBufferDescriptorHeapManager m_RTVDescriptorHeap;
	TwoPartRingBufferDescriptorHeapManager m_DSVDescriptorHeap;
	ConstantBufferDataManager m_ConstantBufferData;

	// UI Stuff
	// TODO: Merge this with main descriptor heap
	ComPtr<ID3D12DescriptorHeap> m_UISRVHeap;
};
}
}