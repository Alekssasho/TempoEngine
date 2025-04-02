#include <CommonIncludes.h>

#include <World/World.h>

#include <World/TaskGraph/TaskGraph.h>

#include <World/Components/Components.h>

#include <World/GameplayFeatures/PhysicsFeature.h>
#include <World/GameplayFeatures/InputControllerFeature.h>
#include <World/GameplayFeatures/SoldierMovementFeature.h>
#include <World/GameplayFeatures/FactorySpawner.h>

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

template<typename Elem, typename Vector = eastl::vector<Elem>>
flecs::opaque<Vector, Elem> std_vector_support(flecs::world& world) {
    return flecs::opaque<Vector, Elem>()
        .as_type(world.vector<Elem>())

        // Forward elements of std::vector value to serializer
        .serialize([](const flecs::serializer *s, const Vector *data) {
            for (const auto& el : *data) {
                s->value(el);
            }
            return 0;
        })

        // Return vector count
        .count([](const Vector *data) {
            return data->size();
        })

        // Resize contents of vector
        .resize([](Vector *data, size_t size) {
            data->resize(size);
        })

        // Ensure element exists, return pointer
        .ensure_element([](Vector *data, size_t elem) {
            if (data->size() <= elem) {
                data->resize(elem + 1);
            }

            return &data->data()[elem];
        });
}

WorldStorage::WorldStorage()
{
	m_EntityWorld.set<flecs::Rest>({});
	m_EntityWorld.import<flecs::stats>();
	// Enable for debug
	//ecs_tracing_enable(3);
	//m_EntityWorld.set_target_fps(60);

	m_EntityWorld.set_task_threads(std::thread::hardware_concurrency());

	// Register all components
	m_EntityWorld.component<glm::quat>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z")
		.member<float>("w");
	m_EntityWorld.component<glm::vec3>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z");
	m_EntityWorld.component<glm::vec4>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z")
		.member<float>("w");

	m_EntityWorld.component<Navigation::LineData>()
		.member<uint32_t>("StartIndex")
		.member<uint32_t>("Count");

	m_EntityWorld.component<eastl::vector<flecs::entity>>()
		.opaque(std_vector_support<flecs::entity>);
	m_EntityWorld.component<eastl::vector<glm::vec3>>()
		.opaque(std_vector_support<glm::vec3>);
	m_EntityWorld.component<eastl::vector<Navigation::LineData>>()
		.opaque(std_vector_support<Navigation::LineData>);

	RegisterComponents(m_EntityWorld);
}

World::World()
{
    //m_Features.emplace_back(new GameplayFeatures::Physics);
    m_Features.emplace_back(new GameplayFeatures::InputController);
    m_Features.emplace_back(new GameplayFeatures::SoldierMovementController);
    m_Features.emplace_back(new GameplayFeatures::FactorySpawner);
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
	// We need to prepare systems first as to add observers if needed.
	// if this is a problem maybe a split will be needed for observers and systems
	for (const auto& feature : m_Features)
	{
		feature->PrepareSystems(*this);
	}

	m_EntityWorld.from_json(data);
	// TODO: This is not working
	eastl::vector<flecs::entity_t> newlyCreatedEntityIds;

	return newlyCreatedEntityIds;
}
}