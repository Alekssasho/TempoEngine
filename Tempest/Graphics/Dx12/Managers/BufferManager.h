#pragma once

#include <EASTL/unordered_map.h>
#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Dx12
{

struct BufferDescription
{
	size_t Size;
	const void* Data;
};

class BufferManager
{
public:
	BufferManager(Dx12Device& device);

	PipelineStateHandle CreateBuffer(const BufferDescription& description);

	ID3D12Resource* GetBuffer(BufferHandle handle);
private:
	ComPtr<ID3D12Resource> CreateStagingBuffer(size_t size);
	void InitializeBufferData(ID3D12Resource* dst, size_t size, const void* data);

	eastl::unordered_map<BufferHandle, ComPtr<ID3D12Resource>> m_Buffers;
	BufferHandle m_NextHandle = 0;
	Dx12Device& m_Device;
};
}
}