#pragma once

#include <EASTL/unique_ptr.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/Dx12/Managers/PipelineManager.h>

namespace Tempest
{
struct RendererCommandList;
namespace Dx12
{
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

	// Managers
	PipelineManager m_PipelineManager;

	// Test Data
	Tempest::Backend::PipelineHandle m_RectsPipelineHandle;
	ComPtr<ID3D12RootSignature> m_Signature;
};
}
}