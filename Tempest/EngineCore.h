#pragma once
#include <CommonIncludes.h>

#include <Job/JobSystem.h>
#include <Platform/WindowsPlatform.h>
#include <Graphics/Renderer.h>
#include <World/World.h>
#include <World/Camera.h>
#include <Resources/ResourceLoader.h>
#include <Audio/AudioManager.h>
#include <Physics/PhysicsManager.h>

#pragma warning(push, 0)
#include <gainput/gainput.h>
#pragma warning(pop)

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

	AudioManager& GetAudio()
	{
		return m_Audio;
	}

	PhysicsManager& GetPhysics()
	{
		return m_Physics;
	}

	Job::JobSystem& GetJobSystem()
	{
		return m_JobSystem;
	}
private:
	// Data members
	EngineCoreOptions m_Options;

	Logger m_Logger;
	Job::JobSystem m_JobSystem;
	gainput::InputManager m_Input;
	gainput::InputMap m_InputMap;
	WindowsPlatform m_Platform;
	ResourceLoader m_ResourceLoader;
	World m_World;
	Renderer m_Renderer;
	Camera m_Camera;
	AudioManager m_Audio;
	PhysicsManager m_Physics;


	// Methods for jobs and executions
	static void InitializeWindowJob(uint32_t, void*);
	void InitializeWindow();

	static void DoFrameJob(uint32_t, void*);
	void UpdateInput();
	void DoFrame();

	static void LoadLevel(const char* levelToLoad);
};

extern EngineCore* gEngine;
}
