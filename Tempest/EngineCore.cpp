#include <CommonIncludes.h>

#include <EngineCore.h>
#include <Memory.h>
#include <optick.h>

#include <imgui.h>

#include <Graphics/FrameData.h>

#include <DataDefinitions/Level_generated.h>

#include <World/Components/Components.h>

namespace Tempest
{
EngineCore* gEngine = nullptr;

EngineCore::EngineCore(const EngineCoreOptions& options)
	: m_Options(options)
	, m_Logger()
	, m_JobSystem(options.NumWorkerThreads, 64, 2 * 1024 * 1024)
	, m_Input(options.Width, options.Height)
	, m_Platform(m_Input.m_Input)
	, m_ResourceLoader(options.ResourceFolder)
	, m_Renderer(options.Renderer)
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

	gEngine->m_JobSystem.WaitSingleJob("Level Load", Job::ThreadTag::Worker, gEngine->m_Options.LevelToLoad, EngineCore::LoadLevel);

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

void EngineCore::LoadLevel(const char* levelToLoad)
{
	const Definition::Level* level = gEngine->GetResourceLoader().LoadResource<Definition::Level>(levelToLoad);
	const char* levelName = level->name()->c_str();
	const char* geometryDatabase = level->geometry_database_file()->c_str();
	const char* textureDatabase = level->texture_database_file()->c_str();
	const char* audioDatabase = level->audio_database_file()->c_str();
	FORMAT_LOG(Info, EngineCore, "Loading Level \"%s\".", levelName);

	// Async Load the rendering databases
	Job::Counter renderingDatabasesCounter;
	struct RenderingDatabases
	{
		const char* geometryDatabase;
		const char* textureDatabase;
	} databases {
		geometryDatabase,
		textureDatabase
	};
	{
		Job::JobDecl loadGeometryDatabase{ [](uint32_t, void* databasesPtr) {
			RenderingDatabases* databases = (RenderingDatabases*)databasesPtr;
			gEngine->GetRenderer().LoadGeometryAndTextureDatabase(databases->geometryDatabase, databases->textureDatabase);
		}, (void*)&databases };
		gEngine->m_JobSystem.RunJobs("Load Geometry Database", &loadGeometryDatabase, 1, &renderingDatabasesCounter);
	}

	// Async Load the audio database
	Job::Counter audioDatabaseCounter;
	{
		Job::JobDecl loadAudioDatabase{ [](uint32_t, void* audioDatabaseName) {
			gEngine->GetAudio().LoadDatabase((const char*)audioDatabaseName);
		}, (void*)audioDatabase };
		gEngine->m_JobSystem.RunJobs("Load Audio Database", &loadAudioDatabase, 1, &audioDatabaseCounter);
	}

	// Runs async to loading the world
	// This needs to be on Windows thread so it is a new job
	{
		Job::JobDecl changeWindowName{ [](uint32_t, void* levelName) {
			gEngine->m_Platform.SetTitleName(reinterpret_cast<const char*>(levelName));
		}, (void*)levelName };
		gEngine->m_JobSystem.RunJobs("Change Window Name", &changeWindowName, 1, nullptr, Job::ThreadTag::Windows);
	}

	// Async Load the physics world
	const flatbuffers::Vector<uint8_t>* physicsData = level->physics_world();
	Job::Counter physicsWorldCounter;
	{
		Job::JobDecl loadPhysicsWorld{ [](uint32_t, void* data) {
			auto dataVector = reinterpret_cast<flatbuffers::Vector<uint8_t>*>(data);
			gEngine->GetPhysics().LoadFromData(dataVector->Data(), dataVector->size());
		}, (void*)physicsData };
		gEngine->m_JobSystem.RunJobs("Load Physics World", &loadPhysicsWorld, 1, &physicsWorldCounter);
	}

	const flatbuffers::Vector<uint8_t>* entitiesData = level->entities();
	const eastl::vector<flecs::entity_t>& newlyCreatedEntities = gEngine->GetWorld().LoadFromLevel(reinterpret_cast<const char*>(entitiesData->Data()), entitiesData->size());

	auto camera = level->camera();

	gEngine->m_Camera.Position = glm::vec3(camera->position().x(), camera->position().y(), camera->position().z());
	gEngine->m_Camera.Forward = glm::normalize(glm::vec3(camera->forward().x(), camera->forward().y(), camera->forward().z()));
	gEngine->m_Camera.Up = glm::vec3(camera->up().x(), camera->up().y(), camera->up().z());
	gEngine->m_Camera.SetPerspectiveProjection(camera->aspect_ratio(), camera->yfov(), camera->znear(), camera->zfar());

	auto cameraController = gEngine->GetWorld().m_EntityWorld.entity("CameraController")
		.set<Components::CameraController>({ gEngine->m_Camera, 0 })
		.set<Components::VehicleController>({ 1 });

	const Components::CameraController* controller = cameraController.get<Components::CameraController>();
	gEngine->m_Renderer.RegisterView(&controller->CameraData);

	// Wait for physics as well
	gEngine->m_JobSystem.WaitForCounter(&physicsWorldCounter, 0);

	// Patch the world with the loaded physics
	gEngine->GetPhysics().PatchWorldComponents(gEngine->GetWorld(), newlyCreatedEntities);

	// Wait for the loading of the rendering databases before initializing it
	gEngine->m_JobSystem.WaitForCounter(&renderingDatabasesCounter, 0);
	gEngine->GetRenderer().InitializeAfterLevelLoad(gEngine->GetWorld());

	// Wait for audio as well
	gEngine->m_JobSystem.WaitForCounter(&audioDatabaseCounter, 0);
}

void EngineCore::InitializeWindow()
{
	m_Platform.SpawnWindow(m_Options.Width, m_Options.Height, "Tempest Engine", this);
	m_Renderer.CreateWindowSurface(m_Platform.GetHandle());
}

void EngineCore::DoFrame()
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