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

class Backend : Utils::NonCopyable
{
public:
	Backend();
	~Backend();

	void Initialize(WindowHandle handle);
	// TODO: It should take Render Graph structure for barriers & a vector of command lists as we will not put everything in one list
	void RenderFrame(const RendererCommandList& commandList);

	Dx12Device* GetDevice() const { return m_Device.get(); }
	UploadData PrepareUpload(uint32_t size);
	void ExecuteUpload(UploadData& uploadData);
private:
	eastl::unique_ptr<Dx12Device> m_Device;
public:
	// Managers
	BackendManagers Managers;
};
}
}
