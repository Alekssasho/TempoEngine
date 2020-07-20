#pragma once
#include <Defines.h>
#include <Logging.h>
#include <Job/JobSystem.h>
#include <Platform/WindowsPlatform.h>
#include <Graphics/Renderer.h>
#include <World/World.h>

namespace Tempest
{
struct EngineCoreOptions
{
	uint32_t NumWorkerThreads;
	uint32_t Width;
	uint32_t Height;
};

class TEMPEST_API EngineCore
{
public:
	EngineCore(const EngineCoreOptions& options);
	~EngineCore();

	void StartEngineLoop();
	void RequestExit();

	World& GetWorld()
	{
		return m_World;
	}
private:
	// Data members
	EngineCoreOptions m_Options;

	Logger m_Logger;
	Job::JobSystem m_JobSystem;
	WindowsPlatform m_Platform;
	Renderer m_Renderer;
	World m_World;

	// Methods for jobs and executions
	static void InitializeWindowJob(void*);
	void InitializeWindow();

	static void DoFrameJob(void*);
	void DoFrame();
};

static EngineCore* gEngine = nullptr;
}