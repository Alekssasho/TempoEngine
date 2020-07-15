#include <EngineCore.h>
#include <Memory.h>
#include <optick.h>

namespace Tempest
{
EngineCore::EngineCore(const EngineCoreOptions& options)
	: m_Options(options)
	, m_Logger()
	, m_JobSystem(options.NumWorkerThreads, 64, 2 * 1024 * 1024)
{
	gEngine = this;
}

EngineCore::~EngineCore()
{
}

void EngineCore::StartEngineLoop()
{
	LOG(Info, EngineCore, "Starting Engine Loop");
	OPTICK_APP("Tempo Engine");
	// This is needed for proper visualization of profile library
	OPTICK_FRAME("Engine Execution");

	Job::JobDecl mainJob{ InitializeWindowJob, this };
	m_JobSystem.RunJobs("Initialize Window Job", &mainJob, 1, nullptr);

	m_JobSystem.WaitForCompletion();

	m_Platform.KillWindow();
}

void EngineCore::RequestExit()
{
	m_JobSystem.Quit();
}

void EngineCore::InitializeWindowJob(void* data)
{
	gEngine->InitializeWindow();

	// Start the engine loop
	Job::JobDecl frameJob{ DoFrameJob, data };
	gEngine->m_JobSystem.RunJobs("Frame", &frameJob, 1, nullptr);
}

void EngineCore::DoFrameJob(void* data)
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

void EngineCore::InitializeWindow()
{
	m_Platform.SpawnWindow(m_Options.Width, m_Options.Height, "Tempest Engine", this);
	m_Renderer.CreateWindowSurface(m_Platform.GetHandle());
}

void EngineCore::DoFrame()
{
	m_Platform.PumpMessages();

}
}