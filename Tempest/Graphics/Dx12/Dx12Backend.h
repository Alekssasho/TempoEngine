#pragma once

#include <EASTL/unique_ptr.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/Dx12/Managers/PipelineManager.h>

namespace Tempest
{
namespace Dx12
{
class Backend
{
public:
	Backend();
	~Backend();

	void Initialize(WindowHandle handle);
	void RenderFrame();
private:
	eastl::unique_ptr<Dx12Device> m_Device;

	// Managers
	PipelineManager m_PipelineManager;

	// Test Data
	PipelineHandle m_MeshPipelineHandle;
};
}
}