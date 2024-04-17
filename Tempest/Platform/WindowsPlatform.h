#pragma once

namespace gainput {
	class InputManager;
}

namespace Tempest
{
using WindowHandle = size_t;
class WindowsPlatform
{
public:
	WindowsPlatform(gainput::InputManager& inputManager);

	void SpawnWindow(unsigned width, unsigned height, const char* title, class Engine* core);
	void PumpMessages();
	void KillWindow();
	void SetTitleName(const char* name);
	WindowHandle GetHandle() { return m_Handle; }
private:
	WindowHandle m_Handle;
	gainput::InputManager& m_InputManager;
};
};