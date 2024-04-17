#pragma once
#include <CommonIncludes.h>

#include <EngineCore.h>
#include <Graphics/Renderer.h>
#include <World/World.h>
#include <World/Camera.h>
#include <Audio/AudioManager.h>
#include <Physics/PhysicsManager.h>

namespace Tempest
{
struct EngineOptions : public EngineCoreOptions
{
    uint32_t Width;
    uint32_t Height;

	Job::JobDecl InitializeDataJob;
};

class TEMPEST_API Engine : public EngineCore
{
public:
	Engine(const EngineOptions& options);
	~Engine();

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

	AudioManager& GetAudio()
	{
		return m_Audio;
	}

	PhysicsManager& GetPhysics()
	{
		return m_Physics;
	}

    InputManager& GetInput()
    {
        return m_Input;
    }

	WindowsPlatform& GetPlatform()
	{
		return m_Platform;
	}

	Camera& GetCamera()
	{
		return m_Camera;
	}

private:
	// Data members
	EngineOptions m_Options;

	InputManager m_Input;
	WindowsPlatform m_Platform;

	World m_World;
	Renderer m_Renderer;
	Camera m_Camera;
	AudioManager m_Audio;
	PhysicsManager m_Physics;

	// Methods for jobs and executions
	static void InitializeWindowJob(uint32_t, void*);
	void InitializeWindow();

	static void DoFrameJob(uint32_t, void*);
	void DoFrame();
};

extern Engine* gEngine;
}
