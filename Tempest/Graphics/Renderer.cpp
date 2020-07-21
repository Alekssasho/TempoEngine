#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <optick.h>

namespace Tempest
{
Renderer::Renderer()
	: m_Backend(new Dx12::Backend)
{
}

Renderer::~Renderer()
{
}

bool Renderer::CreateWindowSurface(WindowHandle handle)
{
	m_Backend->Initialize(handle);
	return false;
}

void Renderer::RenderFrame()
{
	OPTICK_EVENT("Render Frame");
	m_Backend->RenderFrame();
}
}

