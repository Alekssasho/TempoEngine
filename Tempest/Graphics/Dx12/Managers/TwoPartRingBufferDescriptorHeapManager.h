#pragma once

#include <Graphics/Dx12/Dx12Common.h>

namespace Tempest
{
namespace Dx12
{
// Use only a single descriptor heap for all shader visible resources
// Split the memory into 2 pieces - first is for static scene resources which does not change
// They are using simple linear allocation scheme, first are buffers, after that textures for materials, etc
// Second part is dynamic resources, which are allocated per frame, based on ring buffer scheme.
// To avoid having to do synchronizations because multiple frames could be in flight, we allocated
// 1 000 000 descriptors (this is the hardware limit for Tier 1 (lowest tier))
struct TwoPartRingBufferDescriptorHeapManager : Utils::NonCopyable
{
	void Initialize(ID3D12Device3* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity);
	void Destroy();

	// Allocate space for all static resources
	void AllocateStaticResources(int numResources);

	// Allocate a single new dynamic resource
	// This is using ring buffer scheme for allocation
	// NB: You have to first call AllocateStaticResources before calling this method
	uint32_t AllocateDynamicResource();

	ComPtr<ID3D12DescriptorHeap> Heap;
private:
	uint32_t m_Capacity = 0;
	uint32_t m_NumStaticResources = 0;
	uint32_t m_CurrentSlot = 0;
};
}
}