#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/Dx12/Managers/PipelineManager.h>

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
	{}

	PipelineManager Pipeline;
};

class Backend
{
public:
	Backend();
	~Backend();

	void Initialize(WindowHandle handle);
	// TODO: It should take Render Graph structure for barriers & a vector of command lists as we will not put everything in one list
	void RenderFrame(const RendererCommandList& commandList);

private:
	eastl::unique_ptr<Dx12Device> m_Device;
	eastl::array<ComPtr<ID3D12Resource>, 2> m_ConstantBufferData;
public:
	// Managers
	BackendManagers Managers;
};
}
}