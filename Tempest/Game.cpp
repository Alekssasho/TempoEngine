#include <CommonIncludes.h>

#include <Game.h>
#include <DataDefinitions/Level_generated.h>

#include <World/Components/Components.h>

namespace Tempest
{
Game* gGame = nullptr;

Game::Game(const GameOptions& options, const EngineOptions& engineOptions)
	: m_GameOptions(options)
	, m_Engine(PrepareEngineOptions(options, engineOptions))
{
	gGame = this;
}

Game::~Game()
{
}

void Game::Start()
{
	m_Engine.StartEngineLoop();
}

EngineOptions Game::PrepareEngineOptions(const GameOptions& gameOptions, const EngineOptions& inOptions)
{
	EngineOptions outOptions = inOptions;
	outOptions.InitializeDataJob.Data = (void*)gameOptions.LevelToLoad;
	outOptions.InitializeDataJob.EntryPoint = Game::LoadLevel;

	return outOptions;
}

void Game::LoadLevel(uint32_t, void* data)
{
	const char* levelToLoad = (const char*)data;
	const Definition::Level* level = gEngine->GetResourceLoader().LoadResource<Definition::Level>(levelToLoad);
	const char* levelName = level->name()->c_str();
	const char* geometryDatabase = level->geometry_database_file()->c_str();
	const char* textureDatabase = level->texture_database_file()->c_str();
	const char* audioDatabase = level->audio_database_file()->c_str();
	FORMAT_LOG(Info, Game, "Loading Level \"%s\".", levelName);

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
		gEngine->GetJobSystem().RunJobs("Load Geometry Database", &loadGeometryDatabase, 1, &renderingDatabasesCounter);
	}

	// Async Load the audio database
	Job::Counter audioDatabaseCounter;
	{
		Job::JobDecl loadAudioDatabase{ [](uint32_t, void* audioDatabaseName) {
			gEngine->GetAudio().LoadDatabase((const char*)audioDatabaseName);
		}, (void*)audioDatabase };
		gEngine->GetJobSystem().RunJobs("Load Audio Database", &loadAudioDatabase, 1, &audioDatabaseCounter);
	}

	// Runs async to loading the world
	// This needs to be on Windows thread so it is a new job
	{
		Job::JobDecl changeWindowName{ [](uint32_t, void* levelName) {
			gEngine->GetPlatform().SetTitleName(reinterpret_cast<const char*>(levelName));
		}, (void*)levelName };
		gEngine->GetJobSystem().RunJobs("Change Window Name", &changeWindowName, 1, nullptr, Job::ThreadTag::Windows);
	}

	// Async Load the physics world
	const flatbuffers::Vector<uint8_t>* physicsData = level->physics_world();
	Job::Counter physicsWorldCounter;
	{
		Job::JobDecl loadPhysicsWorld{ [](uint32_t, void* data) {
			auto dataVector = reinterpret_cast<flatbuffers::Vector<uint8_t>*>(data);
			gEngine->GetPhysics().LoadFromData(dataVector->Data(), dataVector->size());
		}, (void*)physicsData };
		gEngine->GetJobSystem().RunJobs("Load Physics World", &loadPhysicsWorld, 1, &physicsWorldCounter);
	}

	const flatbuffers::Vector<uint8_t>* entitiesData = level->entities();
	const eastl::vector<flecs::entity_t>& newlyCreatedEntities = gEngine->GetWorld().LoadFromLevel(reinterpret_cast<const char*>(entitiesData->Data()), entitiesData->size());

	auto camera = level->camera();

	gEngine->GetCamera().Position = glm::vec3(camera->position().x(), camera->position().y(), camera->position().z());
	gEngine->GetCamera().Forward = glm::normalize(glm::vec3(camera->forward().x(), camera->forward().y(), camera->forward().z()));
	gEngine->GetCamera().Up = glm::vec3(camera->up().x(), camera->up().y(), camera->up().z());
	gEngine->GetCamera().SetPerspectiveProjection(camera->aspect_ratio(), camera->yfov(), camera->znear(), camera->zfar());

	auto cameraController = gEngine->GetWorld().m_EntityWorld.entity("MainCameraController")
		.set<Components::CameraController>({ gEngine->GetCamera(), 0})
		.set<Components::VehicleController>({ 1 });

	const Components::CameraController* controller = cameraController.get<Components::CameraController>();
	gEngine->GetRenderer().RegisterView(&controller->CameraData);

	// Wait for physics as well
	gEngine->GetJobSystem().WaitForCounter(&physicsWorldCounter, 0);

	// Patch the world with the loaded physics
	gEngine->GetPhysics().PatchWorldComponents(gEngine->GetWorld(), newlyCreatedEntities);

	// Wait for the loading of the rendering databases before initializing it
	gEngine->GetJobSystem().WaitForCounter(&renderingDatabasesCounter, 0);
	gEngine->GetRenderer().InitializeAfterLevelLoad(gEngine->GetWorld());

	// Wait for audio as well
	gEngine->GetJobSystem().WaitForCounter(&audioDatabaseCounter, 0);
}
}