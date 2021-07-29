#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Dx12
{
struct UploadData;

enum class BufferType
{
	Constant,
	Vertex
};

struct BufferDescription
{
	BufferType Type;
	size_t Size;
	const void* Data;
};

class BufferManager : Utils::NonCopyable
{
public:
	BufferManager(Dx12Device& device);

	BufferHandle CreateBuffer(const BufferDescription& description, UploadData* upload);

	ID3D12Resource* GetBuffer(BufferHandle handle);
	void MapWriteData(BufferHandle handle, const void* data, size_t size);

	void DestroyBuffer(BufferHandle handle);

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(BufferHandle handle);
private:
	//ComPtr<ID3D12Resource> CreateStagingBuffer(size_t size);
	//void InitializeBufferData(ID3D12Resource* dst, size_t size, const void* data);

	eastl::unordered_map<BufferHandle, ComPtr<ID3D12Resource>> m_Buffers;
	BufferHandle m_NextHandle = 0;
	Dx12Device& m_Device;
};
}
}