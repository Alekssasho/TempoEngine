#include <CommonIncludes.h>

#include <Graphics/Dx12/Managers/TwoPartRingBufferDescriptorHeapManager.h>

namespace Tempest
{
namespace Dx12
{
void TwoPartRingBufferDescriptorHeapManager::Initialize(ID3D12Device3* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity)
{
	m_Capacity = capacity;

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = m_Capacity;
	srvHeapDesc.Type = type;
	srvHeapDesc.Flags = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // TODO: Sampler heap should also be shader visible
	CHECK_SUCCESS(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&Heap)));
}

void TwoPartRingBufferDescriptorHeapManager::Destroy()
{
	m_Capacity = 0;
	m_NumStaticResources = 0;
	m_CurrentSlot = 0;
	Heap.Reset();
}

void TwoPartRingBufferDescriptorHeapManager::AllocateStaticResources(int numResources)
{
	m_NumStaticResources = numResources;
	m_CurrentSlot = numResources;
}

uint32_t TwoPartRingBufferDescriptorHeapManager::AllocateDynamicResource()
{
	uint32_t allocatedSlot = m_CurrentSlot;

	++m_CurrentSlot;
	// Wrap around as a ring buffer
	if(m_CurrentSlot == m_Capacity)
	{
		m_CurrentSlot = m_NumStaticResources;
	}
	return allocatedSlot;
}
}
}
