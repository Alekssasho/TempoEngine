#pragma once

#include <Defines.h>
#include <EASTL/unique_ptr.h>

#include <Platform/WindowsPlatform.h>

namespace Tempest
{
// This is forward declare and used through a pointer to avoid pulling Dx12 headers into rest of the engine
namespace Dx12 { class Dx12Device; }

class Renderer
{
public:
	Renderer();
	~Renderer();
	bool CreateWindowSurface(WindowHandle handle);
private:
	eastl::unique_ptr<class Dx12::Dx12Device> m_Device;
};
}

