#pragma once

#include <Defines.h>

namespace Tempest
{
using WindowHandle = size_t;
class WindowsPlatform
{
public:
	void SpawnWindow(unsigned width, unsigned height, const char* title, class EngineCore* core);
	void PumpMessages();
	void KillWindow();
	void SetTitleName(const char* name);
	WindowHandle GetHandle() { return m_Handle; }
private:
	WindowHandle m_Handle;
};
};