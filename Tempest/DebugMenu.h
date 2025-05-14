#pragma once

namespace Tempest
{
struct DebugWindow
{
	~DebugWindow() {}
	void Render();

	virtual void DoRender() = 0;

	const char* Name;
	bool Shown = false;
};

struct DebugOptions
{
	DebugOptions();

	eastl::array<float, 6> DeltaTimes;
	float CurrentDeltaTime;
	eastl::vector<eastl::unique_ptr<DebugWindow>> DebugWindows;

	bool ShowNavigationLanes = false;
};
}
