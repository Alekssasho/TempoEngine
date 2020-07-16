#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <optick.h>

#ifdef _DEBUG
#include <initguid.h>
#include <dxgidebug.h>
#endif

namespace Tempest
{
Renderer::Renderer()
	: m_Device(new Dx12::Dx12Device)
{
}

Renderer::~Renderer()
{
	// Reset it by hand to force cleaning of all Dx References and then report live objects to debug
	m_Device.reset();
#ifdef _DEBUG
	{
		ComPtr<IDXGIDebug1> debug;
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	}
#endif
}

bool Renderer::CreateWindowSurface(WindowHandle handle)
{
	m_Device->Initialize(handle);
	return false;
}

void Renderer::RenderFrame()
{
	OPTICK_EVENT("Render Frame");
	Dx12::Dx12FrameData frame = m_Device->StartNewFrame();

	const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	frame.CommandList->ClearRenderTargetView(frame.BackBufferRTV, clearColor, 0, nullptr);

	m_Device->SubmitFrame(frame);

	m_Device->Present();
}
}

