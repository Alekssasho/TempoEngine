#pragma once

#include <Graphics/Dx12/Dx12Common.h>

namespace Tempest
{
namespace Dx12
{

struct ConstantBufferDataManager : Utils::NonCopyable
{
	void Initialize(ID3D12Device3* device);
	void Destroy();

	template<typename T>
	uint32_t AddData(const T& data)
	{
		return AddDataInternal(&data, static_cast<uint32_t>(sizeof(T)));
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress()
	{
		return m_Buffer->GetGPUVirtualAddress();
	}
private:
	uint32_t AddDataInternal(const void* data, uint32_t size);

	ComPtr<ID3D12Resource> m_Buffer;
	uint8_t* m_PersistentMappedMemoryPointer = nullptr;

	uint32_t m_Capacity = 0;
	uint32_t m_CurrentSlot = 0;
	static const uint32_t sAlignment = 256;
};

}
}