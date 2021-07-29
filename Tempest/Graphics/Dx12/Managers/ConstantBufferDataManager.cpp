#include <CommonIncludes.h>

#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>

namespace Tempest
{
namespace Dx12
{
void ConstantBufferDataManager::Initialize(ID3D12Device3* device)
{
	// Somewhat random
	m_Capacity = 100000;

	D3D12_HEAP_PROPERTIES props;
	::ZeroMemory(&props, sizeof(D3D12_HEAP_PROPERTIES));
	props.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = m_Capacity * sAlignment;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CHECK_SUCCESS(device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_Buffer)));

	D3D12_RANGE range;
	::ZeroMemory(&range, sizeof(D3D12_RANGE)); // This will tell that we won't read the data from CPU
	CHECK_SUCCESS(m_Buffer->Map(0, &range, reinterpret_cast<void**>(&m_PersistentMappedMemoryPointer)));
}

void ConstantBufferDataManager::Destroy()
{
	m_Capacity = 0;
	m_CurrentSlot = 0;

	m_Buffer->Unmap(0, nullptr);
	m_Buffer.Reset();
}

uint32_t ConstantBufferDataManager::AddDataInternal(const void* data, uint32_t size)
{
	// TODO: Add checks for to track how many slots are used per frame
	const uint32_t numSlotsRequired = (size + (sAlignment - 1)) / sAlignment;
	if(m_CurrentSlot + numSlotsRequired > m_Capacity)
	{
		m_CurrentSlot = 0; // wrap around directly
	}

	const uint32_t allocatedSlot = m_CurrentSlot;

	m_CurrentSlot += numSlotsRequired;
	// Wrap around as a ring buffer
	if (m_CurrentSlot == m_Capacity)
	{
		m_CurrentSlot = 0;
	}

	memcpy(m_PersistentMappedMemoryPointer + (allocatedSlot * sAlignment), data, size);

	return allocatedSlot * sAlignment;
}
}
}
