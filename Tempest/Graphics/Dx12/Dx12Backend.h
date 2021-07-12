#pragma once

#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/Dx12/Managers/PipelineManager.h>
#include <Graphics/Dx12/Managers/BufferManager.h>
#include <Graphics/Dx12/Managers/TextureManager.h>
#include <World/Camera.h>

namespace Tempest
{
struct RendererCommandList;
struct RenderManagers;
namespace Dx12
{

struct BackendManagers
{
	BackendManagers(Dx12Device& device)
		: Pipeline(device)
		, Buffer(device)
		, Texture(device)
	{}

	PipelineManager Pipeline;
	BufferManager Buffer;
	TextureManager Texture;
};

struct UploadData
{
	ComPtr<ID3D12CommandAllocator> Allocator;
	ComPtr<ID3D12GraphicsCommandList6> CommandList;
	ComPtr<ID3D12Resource> UploadHeap;
	void* MappedData;
	uint64_t CurrentOffset;
};

class Backend
{
public:
	Backend();
	~Backend();

	void Initialize(WindowHandle handle);
	// TODO: It should take Render Graph structure for barriers & a vector of command lists as we will not put everything in one list
	// TODO: Remove Camera and frame data as this are not concepts backend should be concerned with
	void RenderFrame(const Camera* view, const FrameData& frameData, const RendererCommandList& commandList);

	Dx12Device* GetDevice() const { return m_Device.get(); }
	UploadData PrepareUpload(uint32_t size);
	void ExecuteUpload(UploadData& uploadData);
private:
	eastl::unique_ptr<Dx12Device> m_Device;

	// Use more than one buffer for every backbuffer index to avoid hazards
	using ConstantBufferHandle = eastl::array<BufferHandle, 2>;
	ConstantBufferHandle m_GeometryConstantBufferData = { sInvalidHandle, sInvalidHandle };
	ConstantBufferHandle m_SceneConstantBufferData = { sInvalidHandle, sInvalidHandle };
public:
	// Managers
	BackendManagers Managers;
};
}
}
