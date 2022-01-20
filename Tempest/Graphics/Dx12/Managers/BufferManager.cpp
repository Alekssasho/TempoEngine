#include <CommonIncludes.h>

#include <Graphics/Dx12/Managers/BufferManager.h>
#include <Graphics/Dx12/Dx12Backend.h>

namespace Tempest
{
namespace Dx12
{
BufferManager::BufferManager(Dx12Device& device)
	: m_Device(device)
{
}

static D3D12_HEAP_TYPE GetHeapTypeFromBufferType(BufferType type)
{
	switch (type)
	{
	case BufferType::Constant:
		return D3D12_HEAP_TYPE_UPLOAD;
	case BufferType::Vertex:
	case BufferType::UAV:
		return D3D12_HEAP_TYPE_DEFAULT;
	default: assert(false);
	}

	return D3D12_HEAP_TYPE_DEFAULT;
}

BufferHandle BufferManager::CreateBuffer(const BufferDescription& description, UploadData* upload)
{
	D3D12_HEAP_PROPERTIES props;
	::ZeroMemory(&props, sizeof(D3D12_HEAP_PROPERTIES));
	props.Type = GetHeapTypeFromBufferType(description.Type);

	D3D12_RESOURCE_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = description.Size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	PipelineStateHandle resultHandle = m_NextHandle++;
	ComPtr<ID3D12Resource>& buffer = m_Buffers[resultHandle];
	D3D12_RESOURCE_STATES state = description.Data ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
	CHECK_SUCCESS(m_Device.GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, NULL, IID_PPV_ARGS(&buffer)));

	if(description.Data && upload)
	{
		memcpy(reinterpret_cast<uint8_t*>(upload->MappedData) + upload->CurrentOffset, description.Data, description.Size);

		upload->CommandList->CopyBufferRegion(buffer.Get(), 0, upload->UploadHeap.Get(), upload->CurrentOffset, description.Size);
		upload->CurrentOffset += description.Size;

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = buffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		// TODO: Batch those
		upload->CommandList->ResourceBarrier(1, &barrier);
	}

	return resultHandle;
}

ID3D12Resource* BufferManager::GetBuffer(BufferHandle handle)
{
	return m_Buffers[handle].Get();
}

void BufferManager::MapWriteData(BufferHandle handle, const void* data, size_t size)
{
	ID3D12Resource* buffer = GetBuffer(handle);
	assert(buffer);

	D3D12_RANGE range;
	::ZeroMemory(&range, sizeof(D3D12_RANGE)); // This will tell that we won't read the data from CPU
	void* dataPointer = nullptr;
	CHECK_SUCCESS(buffer->Map(0, &range, &dataPointer));

	memcpy(dataPointer, data, size);

	buffer->Unmap(0, nullptr);
}

void BufferManager::DestroyBuffer(BufferHandle handle)
{
	m_Buffers.erase(handle);
}

D3D12_GPU_VIRTUAL_ADDRESS BufferManager::GetGPUAddress(BufferHandle handle)
{
	return m_Buffers[handle]->GetGPUVirtualAddress();
}

//ComPtr<ID3D12Resource> BufferManager::CreateStagingBuffer(size_t size)
//{
//	D3D12_HEAP_PROPERTIES props;
//	::ZeroMemory(&props, sizeof(D3D12_HEAP_PROPERTIES));
//	props.Type = D3D12_HEAP_TYPE_UPLOAD;
//
//	D3D12_RESOURCE_DESC desc;
//	::ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
//	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	desc.Width = size;
//	desc.Height = 1;
//	desc.DepthOrArraySize = 1;
//	desc.MipLevels = 1;
//	desc.Format = DXGI_FORMAT_UNKNOWN;
//	desc.SampleDesc.Count = 1;
//	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
//
//	ComPtr<ID3D12Resource> buffer;
//	CHECK_SUCCESS(m_Device.GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&buffer)));
//	return buffer;
//}

//void BufferManager::InitializeBufferData(ID3D12Resource* dst, size_t size, const void* data)
//{
//	ComPtr<ID3D12Resource> stagingBuffer = CreateStagingBuffer(size);
//	D3D12_RANGE range;
//	::ZeroMemory(&range, sizeof(D3D12_RANGE));
//	void* stagingBufferData = nullptr;
//	CHECK_SUCCESS(stagingBuffer->Map(0, &range, &stagingBufferData));
//
//	memcpy(stagingBufferData, data, size);
//
//	stagingBuffer->Unmap(0, nullptr);
//
//	m_Device.CopyResources(dst, stagingBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
//}
}
}