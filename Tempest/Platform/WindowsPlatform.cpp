#include <CommonIncludes.h>

#include <Platform/WindowsPlatform.h>

#include <EngineCore.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <imgui.h>

static bool g_MouseJustPressed[3] = { false, false, false };
static bool g_MouseCurrentState[3] = { false, false, false };

static Tempest::EngineCore* GetEngine(HWND hWnd)
{
	LONG_PTR ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
	assert(ptr);
	return reinterpret_cast<Tempest::EngineCore*>(ptr);
}

LRESULT CALLBACK DefaultWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto& io = ImGui::GetIO();
	switch (message)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		auto engineCore = reinterpret_cast<Tempest::EngineCore*>(pCreate->lpCreateParams);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)engineCore);
		break;
	}
	case WM_DESTROY:
	{
		GetEngine(hWnd)->RequestExit();
		::PostQuitMessage(0);
		return 0;
	}
	case WM_KEYDOWN:
		if (wParam < 256)
		{
			io.KeysDown[wParam] = 1;
		}

	break;
	case WM_KEYUP:
		if (wParam < 256)
		{
			io.KeysDown[wParam] = 0;
		}

	break;
	case WM_CHAR:
		if (wParam > 0 && wParam < 0x10000)
		{
			io.AddInputCharacter((unsigned short)wParam);
		}
		break;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);

	break;
	case WM_LBUTTONDOWN:
		g_MouseJustPressed[0] = true;
		g_MouseCurrentState[0] = true;
	break;
	case WM_LBUTTONUP:
		g_MouseCurrentState[0] = false;
	break;
	case WM_RBUTTONDOWN:
		g_MouseJustPressed[1] = true;
		g_MouseCurrentState[1] = true;
	break;
	case WM_RBUTTONUP:
		g_MouseCurrentState[1] = false;
	break;
	case WM_MBUTTONDOWN:
		g_MouseJustPressed[2] = true;
		g_MouseCurrentState[2] = true;
	break;
	case WM_MBUTTONUP:
		g_MouseCurrentState[2] = false;
	break;
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

namespace Tempest
{

bool g_Run = true;

void WindowsPlatform::SpawnWindow(unsigned width, unsigned height, const char* title, EngineCore* core)
{
	ImGui::CreateContext();

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
		core);

	auto res = ::ShowWindow(hWnd, SW_RESTORE);

	// UI
	{
		// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';
	}

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
	}

	// UI
	{
		ImGuiIO& io = ImGui::GetIO();
		for (int i = 0; i < 3; i++)
		{
			// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
			io.MouseDown[i] = g_MouseJustPressed[i] || g_MouseCurrentState[i];
			g_MouseJustPressed[i] = false;
		}

		io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
		io.KeySuper = false;
	}
}

void WindowsPlatform::KillWindow()
{
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