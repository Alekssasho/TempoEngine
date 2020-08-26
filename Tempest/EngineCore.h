#pragma once
#include <Defines.h>
#include <Logging.h>
#include <Job/JobSystem.h>
#include <Platform/WindowsPlatform.h>
#include <Graphics/Renderer.h>
#include <World/World.h>
#include <Resources/ResourceLoader.h>

namespace Tempest
{
struct EngineCoreOptions
{
	uint32_t NumWorkerThreads;
	uint32_t Width;
	uint32_t Height;
	const char* ResourceFolder;
	const char* LevelToLoad;
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

	ResourceLoader& GetResourceLoader()
	{
		return m_ResourceLoader;
	}
private:
	// Data members
	EngineCoreOptions m_Options;

	Logger m_Logger;
	Job::JobSystem m_JobSystem;
	WindowsPlatform m_Platform;
	ResourceLoader m_ResourceLoader;
	Renderer m_Renderer;
	World m_World;

	// Methods for jobs and executions
	static void InitializeWindowJob(void*);
	void InitializeWindow();

	static void DoFrameJob(void*);
	void DoFrame();
};

extern EngineCore* gEngine;
}