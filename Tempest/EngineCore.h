#pragma once
#include <Defines.h>
#include <Logging.h>
#include <Job/JobSystem.h>
#include <Platform/WindowsPlatform.h>
#include <Graphics/Renderer.h>
#include <World/World.h>
#include <World/Camera.h>
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

	Renderer& GetRenderer()
	{
		return m_Renderer;
	}

	ResourceLoader& GetResourceLoader()
	{
		return m_ResourceLoader;
	}

	EngineCoreOptions& GetOptions()
	{
		return m_Options;
	}
private:
	// Data members
	EngineCoreOptions m_Options;

	Logger m_Logger;
	Job::JobSystem m_JobSystem;
	WindowsPlatform m_Platform;
	ResourceLoader m_ResourceLoader;
	World m_World;
	Renderer m_Renderer;
	Camera m_Camera;

	// Methods for jobs and executions
	static void InitializeWindowJob(uint32_t, void*);
	void InitializeWindow();

	static void DoFrameJob(uint32_t, void*);
	void UpdateInput();
	void DoFrame();
};

extern EngineCore* gEngine;
}
