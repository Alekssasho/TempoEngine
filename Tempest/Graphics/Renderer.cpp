#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Device.h>

namespace Tempest
{
Renderer::Renderer()
	: m_Device(new Dx12::Dx12Device)
{
}

Renderer::~Renderer()
{
}

bool Renderer::CreateWindowSurface(WindowHandle handle)
{
	m_Device->Initialize(handle);
	return false;
}
}

