#include <CommonIncludes.h>

#include <Graphics/Dx12/MainDescriptorHeapManager.h>

namespace Tempest
{
namespace Dx12
{
void MainDescriptorHeapManager::Initialize(ID3D12Device3* device)
{
	// Hardware Tier 1 limit
	m_Capacity = 1000000;
	
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = m_Capacity;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CHECK_SUCCESS(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&Heap)));
}

void MainDescriptorHeapManager::Destroy()
{
	m_Capacity = 0;
	m_NumStaticResources = 0;
	m_CurrentSlot = 0;
	Heap.Reset();
}

void MainDescriptorHeapManager::AllocateStaticResources(int numResources)
{
	m_NumStaticResources = numResources;
	m_CurrentSlot = numResources;
}

uint32_t MainDescriptorHeapManager::AllocateDynamicResource()
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
