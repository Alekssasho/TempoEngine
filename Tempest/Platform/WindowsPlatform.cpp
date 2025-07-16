#include <CommonIncludes.h>

#include <Platform/WindowsPlatform.h>

#include <Engine.h>

#include <Windows.h>

#include <imgui.h>
#include <imgui_impl_win32.h>

#pragma warning(push, 0)
#include <gainput/gainput.h>
#pragma warning(pop)

static bool g_MouseJustPressed[3] = { false, false, false };
static bool g_MouseCurrentState[3] = { false, false, false };

static Tempest::Engine* GetEngine(HWND hWnd)
{
	LONG_PTR ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
	assert(ptr);
	return reinterpret_cast<Tempest::Engine*>(ptr);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK DefaultWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

	switch (message)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		auto engine = reinterpret_cast<Tempest::Engine*>(pCreate->lpCreateParams);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)engine);
		break;
	}
	case WM_DESTROY:
	{
		GetEngine(hWnd)->RequestExit();
		::PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

namespace Tempest
{

bool g_Run = true;

WindowsPlatform::WindowsPlatform(gainput::InputManager& inputManager)
	: m_InputManager(inputManager)
{
}

void WindowsPlatform::SpawnWindow(unsigned width, unsigned height, const char* title, Engine* engine)
{
	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	ImGui::LoadIniSettingsFromDisk("imgui_settings.ini");

	auto hIntance = ::GetModuleHandle(NULL);
	WNDCLASSEX wc;
	::ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DefaultWindowProc;
	wc.hInstance = hIntance;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "TempestWindowClass";

	::RegisterClassEx(&wc);

	auto hWnd = ::CreateWindowEx(NULL,
		"TempestWindowClass",
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		hIntance,
		engine);

	auto res = ::ShowWindow(hWnd, SW_RESTORE);

	ImGui_ImplWin32_Init(hWnd);

	// Enable virtual terminal support for colored output
	auto enableVirtualTerminal = [](HANDLE handle)
	{
		DWORD dwMode = 0;
		if (!GetConsoleMode(handle, &dwMode))
			return;

		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(handle, dwMode);
	};

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	enableVirtualTerminal(hOut);
	HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
	enableVirtualTerminal(hErr);

	m_Handle = WindowHandle(hWnd);
}

void WindowsPlatform::PumpMessages()
{
	MSG msg;

	while(::PeekMessage(&msg, HWND(m_Handle), 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

		m_InputManager.HandleMessage(msg);
	}
}

void WindowsPlatform::KillWindow()
{
	ImGui::SaveIniSettingsToDisk("imgui_settings.ini");
	ImGui_ImplWin32_Shutdown();
	::DestroyWindow(HWND(m_Handle));
}

void WindowsPlatform::SetTitleName(const char* name)
{
	eastl::string buffer;
	buffer.sprintf("Tempest Engine - %s", name);
	::SetWindowText(HWND(m_Handle), buffer.c_str());
}
};

// Needed by EASTL to function properly
#include <stdio.h>
int Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments)
{
	return ::vsnprintf(pDestination, n, pFormat, arguments);
}

int VsnprintfW(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
{
	return ::vswprintf(pDestination, n, pFormat, arguments);
}