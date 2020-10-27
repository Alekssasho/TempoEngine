#include <EngineCore.h>
#include <Memory.h>
#include <optick.h>

#include <imgui.h>

#include <Graphics/FrameData.h>

#include <DataDefinitions/Level_generated.h>

namespace Tempest
{
EngineCore* gEngine = nullptr;

EngineCore::EngineCore(const EngineCoreOptions& options)
	: m_Options(options)
	, m_Logger()
	, m_JobSystem(options.NumWorkerThreads, 64, 2 * 1024 * 1024)
	, m_ResourceLoader(options.ResourceFolder)
{
	gEngine = this;

	m_Camera.SetCamera(glm::vec3(3.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_Camera.SetPerspectiveProjection(float(options.Width) / float(options.Height), glm::radians(90.0f), 0.1f, 1000.0f);
	m_Renderer.RegisterView(&m_Camera);
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
	m_JobSystem.RunJobs("Initialize Window Job", &mainJob, 1, nullptr, Job::ThreadTag::Windows);

	m_JobSystem.WaitForCompletion();

	m_Platform.KillWindow();
}

void EngineCore::RequestExit()
{
	m_JobSystem.Quit();
}

void EngineCore::InitializeWindowJob(uint32_t, void* data)
{
	gEngine->InitializeWindow();

	gEngine->m_JobSystem.WaitSingleJob("Level Load", Job::ThreadTag::Worker, gEngine->m_Options.LevelToLoad, [](const char* levelToLoad) {
		const Definition::Level* level = gEngine->GetResourceLoader().LoadResource<Definition::Level>(levelToLoad);
		const char* levelName = level->name()->c_str();
		const char* geometryDatabase = level->geometry_database_file()->c_str();
		FORMAT_LOG(Info, EngineCore, "Loading Level \"%s\".", levelName);

		// Async Load the geometry database
		Job::Counter geometryDatabaseCounter;
		{
			Job::JobDecl loadGeometryDatabase{ [](uint32_t, void* geometryDatabaseName) {
				gEngine->GetRenderer().LoadGeometryDatabase((const char*)geometryDatabaseName);
			}, (void*)geometryDatabase };
			gEngine->m_JobSystem.RunJobs("Load Geometry Database", &loadGeometryDatabase, 1, &geometryDatabaseCounter);
		}

		// Runs async to loading the world
		{
			Job::JobDecl changeWindowName{ [](uint32_t, void* levelName) {
				gEngine->m_Platform.SetTitleName(reinterpret_cast<const char*>(levelName));
			}, (void*)levelName };
			gEngine->m_JobSystem.RunJobs("Change Window Name", &changeWindowName, 1, nullptr, Job::ThreadTag::Windows);
		}

		const flatbuffers::Vector<uint8_t>* entitiesData = level->entities();
		gEngine->GetWorld().LoadFromLevel(reinterpret_cast<const char*>(entitiesData->Data()), entitiesData->size());

		// Wait for the loading of the geometry before initializing it
		gEngine->m_JobSystem.WaitForCounter(&geometryDatabaseCounter, 0);
		gEngine->GetRenderer().InitializeAfterLevelLoad(gEngine->GetWorld());
	});

	// Start the engine loop
	Job::JobDecl frameJob{ DoFrameJob, data };
	gEngine->m_JobSystem.RunJobs("Frame", &frameJob, 1, nullptr);
}

void EngineCore::DoFrameJob(uint32_t, void* data)
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

	// TODO: add real delta time
	m_World.Update(1.0f / 60.0f, m_JobSystem);

	// TODO: This should be on seperate job and be pipelined with the DoFrame job
	FrameData frameData = m_Renderer.GatherWorldData(m_World);
	m_Renderer.RenderFrame(frameData);
}
}