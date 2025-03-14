#include <CommonIncludes.h>

#include <World/World.h>

#include <World/TaskGraph/TaskGraph.h>

#include <World/Components/Components.h>

#include <World/GameplayFeatures/PhysicsFeature.h>
#include <World/GameplayFeatures/InputControllerFeature.h>
#include <World/GameplayFeatures/SoldierMovementFeature.h>

template<typename ComponentType>
void RegisterComponent(flecs::world& world)
{
	flecs::component<ComponentType>(world, ComponentType::Name);
}

namespace Tempest
{

struct TaskData
{
	ecs_os_thread_callback_t Callback;
	void* CallbackParams;
	Job::Counter WaitCounter;
};

static ecs_os_thread_t EcsNewTask(ecs_os_thread_callback_t callback, void* param)
{
    TaskData* data = new TaskData;
    data->Callback = callback;
    data->CallbackParams = param;
    Job::JobDecl ecsTask{ [](uint32_t, void* taskData) {
            TaskData* data = (TaskData*)taskData;
            data->Callback(data->CallbackParams);
        }, (void*)data };
    gEngine->GetJobSystem().RunJobs("Ecs Task", &ecsTask, 1, &data->WaitCounter);
    return (ecs_os_thread_t)data;
}

static void* EcsWaitTask(ecs_os_thread_t taskData)
{
    TaskData* data = (TaskData*)taskData;
    gEngine->GetJobSystem().WaitForCounter(&data->WaitCounter, 0);
    delete data;
	return nullptr;
}

FlecsIniter::FlecsIniter()
{
    ecs_os_set_api_defaults();
    ecs_os_api_t api = ecs_os_get_api();
    api.task_new_ = EcsNewTask;
    api.task_join_ = EcsWaitTask;
    ecs_os_set_api(&api);
}

WorldStorage::WorldStorage()
{
	// Enable for debug
	//ecs_tracing_enable(3);
	//m_EntityWorld.set_target_fps(60);

	m_EntityWorld.set_task_threads(std::thread::hardware_concurrency());

	// Register all components
	//RegisterComponent<Components::Transform>(m_EntityWorld);
	m_EntityWorld.component<glm::quat>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z")
		.member<float>("w");
    m_EntityWorld.component<glm::vec3>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z");
    m_EntityWorld.component<Components::Transform>(Components::Transform::Name)
        .member<glm::quat>("Rotation")
		.member<glm::vec3>("Position")
		.member<glm::vec3>("Scale");
	RegisterComponent<Components::Rect>(m_EntityWorld);
	//RegisterComponent<Components::StaticMesh>(m_EntityWorld);
	m_EntityWorld.component<Components::StaticMesh>(Components::StaticMesh::Name)
		.member<uint32_t>("Mesh");
	RegisterComponent<Components::CameraController>(m_EntityWorld);
	RegisterComponent<Components::CarPhysicsPart>(m_EntityWorld);
	RegisterComponent<Components::DynamicPhysicsActor>(m_EntityWorld);
	//RegisterComponent<Components::LightColorInfo>(m_EntityWorld);
    m_EntityWorld.component<Components::LightColorInfo>(Components::LightColorInfo::Name)
        .member<glm::vec3>("Color")
        .member<float>("Intensity");
	RegisterComponent<Components::VehicleController>(m_EntityWorld);
	RegisterComponent<Components::Faction>(m_EntityWorld);

	RegisterComponent<Tags::Boids>(m_EntityWorld);
	RegisterComponent<Tags::DirectionalLight>(m_EntityWorld);
}

World::World()
{
    //m_Features.emplace_back(new GameplayFeatures::Physics);
    m_Features.emplace_back(new GameplayFeatures::InputController);
    m_Features.emplace_back(new GameplayFeatures::SoldierMovementController);
}

World::~World()
{
}

void World::Update(float deltaTime, Job::JobSystem& jobSystem)
{
	OPTICK_EVENT();
	m_EntityWorld.progress(deltaTime);
}

class MemoryInputStream
{
public:
	MemoryInputStream(const uint8_t* buffer, uint64_t bufferSize)
		: m_ReaderPosition(0)
		, m_BufferSize(bufferSize)
		, m_Buffer(buffer)
	{}

	const uint8_t* Jump(uint64_t bytesToJump)
	{
		assert(m_ReaderPosition + bytesToJump <= m_BufferSize);
		auto result = m_Buffer + m_ReaderPosition;
		m_ReaderPosition += bytesToJump;
		return result;
	}

	void Read(uint8_t* bufferToFill, uint64_t bytesToRead)
	{
		assert(m_ReaderPosition + bytesToRead <= m_BufferSize);
		std::memcpy(bufferToFill, m_Buffer + m_ReaderPosition, bytesToRead);
		m_ReaderPosition += bytesToRead;
	}

	template<typename T>
	void Read(T& value)
	{
		value = *reinterpret_cast<const T*>(m_Buffer + m_ReaderPosition);
		m_ReaderPosition += sizeof(T);
	}
private:
	uint64_t m_ReaderPosition;
	uint64_t m_BufferSize;
	const uint8_t* m_Buffer;
};

eastl::vector<flecs::entity_t> World::LoadFromLevel(const char* data, size_t size)
{
	m_EntityWorld.from_json(data);
	eastl::vector<flecs::entity_t> newlyCreatedEntityIds;

	//MemoryInputStream stream(reinterpret_cast<const uint8_t*>(data), size);

	//uint32_t numArchetypes;
	//stream.Read(numArchetypes);

	//for (uint32_t i = 0; i < numArchetypes; ++i) {
	//	uint32_t numEntities;
	//	stream.Read(numEntities);

	//	// Read the number of components
	//	uint32_t numComponents;
	//	stream.Read(numComponents);

	//	// TODO: Temp memory
	//	eastl::vector<uint32_t> componentSizes;
	//	componentSizes.resize(numComponents);
	//	stream.Read(reinterpret_cast<uint8_t*>(componentSizes.data()), numComponents * sizeof(uint32_t));

	//	uint32_t nameLen;
	//	stream.Read(nameLen);

	//	auto archetypeName = reinterpret_cast<const char*>(stream.Jump(nameLen));

	//	eastl::vector<const void*> componentArrayPointers;
	//	componentArrayPointers.reserve(numComponents);

	//	for (auto size : componentSizes) {
	//		// This is a tag, so we just push a nullptr
	//		if (size == 0) {
	//			componentArrayPointers.push_back(nullptr);
	//		}
	//		else
	//		{
	//			componentArrayPointers.push_back(stream.Jump(numEntities * size));
	//		}
	//	}

	//	const uint32_t beforeSize = uint32_t(newlyCreatedEntityIds.size());
	//	newlyCreatedEntityIds.resize(beforeSize + numEntities);

	//	ecs_bulk_desc_t desc = {};
	//	desc.count = numEntities;
	//	desc.data = const_cast<void**>(componentArrayPointers.data());
	//	//desc.table = ecs_table_from_str(m_EntityWorld, archetypeName);

	//	uint32_t currentTermCount = 0;
	//	ecs_term_t currentTerm;
	//	while (archetypeName[0] && (archetypeName = ecs_parse_term(m_EntityWorld, nullptr, nullptr, archetypeName, &currentTerm, nullptr, nullptr, false)))
	//	{
	//		ecs_term_finalize(m_EntityWorld, &currentTerm);
	//		desc.ids[currentTermCount] = currentTerm.id;
	//		++currentTermCount;
	//		ecs_term_fini(&currentTerm);
	//	}
	//	//desc.table = ecs_table_find(m_EntityWorld, )

	//	const ecs_entity_t* newlyCreatedIds = ecs_bulk_init(m_EntityWorld, &desc);
	//	memcpy(newlyCreatedEntityIds.data() + beforeSize, newlyCreatedIds, numEntities * sizeof(ecs_entity_t));
	//}

	for (const auto& feature : m_Features)
	{
		feature->PrepareSystems(*this);
	}

	return newlyCreatedEntityIds;
}
}