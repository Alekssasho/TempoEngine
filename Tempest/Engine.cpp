#include <CommonIncludes.h>

#include <Engine.h>
#include <optick.h>

#include <imgui.h>

#include <Graphics/FrameData.h>

namespace Tempest
{
Engine* gEngine = nullptr;

Engine::Engine(const EngineOptions& options)
	: EngineCore(options)
	, m_Options(options)
    , m_Input(options.Width, options.Height)
    , m_Platform(m_Input.m_Input)
{
	gEngine = this;
}

Engine::~Engine()
{
}

void Engine::StartEngineLoop()
{
	LOG(Info, Engine, "Starting Engine Loop");
	OPTICK_APP("Tempo Engine");
	// This is needed for proper visualization of profile library
	OPTICK_FRAME("Engine Execution");

	Job::JobDecl mainJob{ InitializeWindowJob, this };
	m_JobSystem.RunJobs("Initialize Window Job", &mainJob, 1, nullptr, Job::ThreadTag::Windows);

	m_JobSystem.WaitForCompletion();

	m_Platform.KillWindow();
}

void Engine::RequestExit()
{
	m_JobSystem.Quit();
}

void Engine::InitializeWindowJob(uint32_t, void* data)
{
	gEngine->InitializeWindow();

	Job::Counter counter;
	gEngine->m_JobSystem.RunJobs("Initialize Data", &gEngine->m_Options.InitializeDataJob, 1, &counter, Job::ThreadTag::Worker);
	gEngine->m_JobSystem.WaitForCounter(&counter, 0);

	// Start the engine loop
	Job::JobDecl frameJob{ DoFrameJob, data };
	gEngine->m_JobSystem.RunJobs("Frame", &frameJob, 1, nullptr);
}

void Engine::DoFrameJob(uint32_t, void* data)
{
	// Start a new frame and update the profiler
	OPTICK_UPDATE();
	Optick::BeginFrame();

	gEngine->DoFrame();

	// End the current frame in the profiler
	Optick::EndFrame();

	// Schedule frame again. This should be the last thing happening in this frame.
	Job::JobDecl frameJob{ DoFrameJob, data };
	gEngine->m_JobSystem.RunJobs("Frame", &frameJob, 1, nullptr);
}

void Engine::InitializeWindow()
{
	m_Platform.SpawnWindow(m_Options.Width, m_Options.Height, "Tempest Engine", this);
	m_Renderer.CreateWindowSurface(m_Platform.GetHandle());
}

void Engine::DoFrame()
{
	// TODO: pass delta time
	m_Input.Update(0.0f);

	// Message pumping should be done on the Windows Thread
	m_JobSystem.WaitSingleJob("Pump Messages", Job::ThreadTag::Windows, m_Platform, [](WindowsPlatform& platform) {
		platform.PumpMessages();
	});

	// UI
	{
		auto& io = ImGui::GetIO();
		//io.DeltaTime = deltaTime;
		ImGui::NewFrame();
	}

	// Test Code
	//ImGui::ShowDemoWindow();
	//ImGui::Begin("Test");
	//ImGui::Checkbox("Perspective Projection Negative To One", &m_Camera.proper);
	//ImGui::DragFloat("Znear", &m_Camera.znear);
	//ImGui::End();


	// TODO: add real delta time
	m_World.Update(1.0f / 60.0f, m_JobSystem);

	// Audio
	m_Audio.Update();

	// TODO: This should be on seperate job and be pipelined with the DoFrame job
	FrameData frameData = m_Renderer.GatherWorldData(m_World);
	m_Renderer.RenderFrame(frameData);
}
}