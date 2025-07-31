#include <CommonIncludes.h>

#include <World/World.h>

#include <World/TaskGraph/TaskGraph.h>

#include <World/Components/Components.h>

#include <World/GameplayFeatures/PhysicsFeature.h>
#include <World/GameplayFeatures/InputControllerFeature.h>
#include <World/GameplayFeatures/SoldierMovementFeature.h>
#include <World/GameplayFeatures/FactorySpawner.h>
#include <World/GameplayFeatures/SpaceLocation.h>
#include <World/GameplayFeatures/BattleFeature.h>
#include <World/GameplayFeatures/HealthManagementFeature.h>
#include <World/GameplayFeatures/AnimationController.h>
#include <World/GameplayFeatures/SoundFeature.h>

namespace Tempest
{
uint64_t GetGlobalSeed()
{
	return 1234;
}

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

static thread_local eastl::stack<TracyCZoneCtx> s_FlecsTracyStack;

static void EcsPushMarker(const char* file, size_t line, const char* name)
{
	uint64_t srcLoc = ___tracy_alloc_srcloc(uint32_t(line), file, strlen(file), name, strlen(name), 0);
	s_FlecsTracyStack.push(___tracy_emit_zone_begin_alloc(srcLoc, true));
}

static void EcsPopMarker(const char* file, size_t line, const char* name)
{
	TracyCZoneCtx ctx = s_FlecsTracyStack.top();
	s_FlecsTracyStack.pop();

	___tracy_emit_zone_end(ctx);
}

FlecsIniter::FlecsIniter()
{
    ecs_os_set_api_defaults();
    ecs_os_api_t api = ecs_os_get_api();
    api.task_new_ = EcsNewTask;
    api.task_join_ = EcsWaitTask;
	// TODO: We are using vcpkg build of flecs which we cannot enable this for now
    //api.perf_trace_push_ = EcsPushMarker;
    //api.perf_trace_pop_ = EcsPopMarker;
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
	ecs_set_task_threads(m_EntityWorld, gEngineCore->GetOptions().NumWorkerThreads);
	m_EntityWorld.set<flecs::Rest>({});
	m_EntityWorld.import<flecs::stats>();
	// Enable for debug
	//ecs_tracing_enable(3);
	//m_EntityWorld.set_target_fps(60);

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
    m_EntityWorld.component<glm::mat4x4>()
        .member<float>("x0")
        .member<float>("y0")
        .member<float>("z0")
        .member<float>("w0")
        .member<float>("x1")
        .member<float>("y1")
        .member<float>("z1")
        .member<float>("w1")
        .member<float>("x2")
        .member<float>("y2")
        .member<float>("z2")
        .member<float>("w2")
        .member<float>("x3")
        .member<float>("y3")
        .member<float>("z3")
        .member<float>("w3");

	m_EntityWorld.component<Navigation::LineData>()
		.member<uint32_t>("StartIndex")
		.member<uint32_t>("Count");

	m_EntityWorld.component<eastl::vector<flecs::entity>>()
		.opaque(std_vector_support<flecs::entity>);
	m_EntityWorld.component<eastl::vector<glm::vec3>>()
		.opaque(std_vector_support<glm::vec3>);
    m_EntityWorld.component<eastl::vector<glm::mat4x4>>()
        .opaque(std_vector_support<glm::mat4x4>);
	m_EntityWorld.component<eastl::vector<Navigation::LineData>>()
		.opaque(std_vector_support<Navigation::LineData>);
    m_EntityWorld.component<eastl::vector<uint32_t>>()
        .opaque(std_vector_support<uint32_t>);

	RegisterComponents(m_EntityWorld);
}

World::World()
{
    //m_Features.emplace_back(new GameplayFeatures::Physics);
    m_Features.emplace_back(new GameplayFeatures::InputController);
    m_Features.emplace_back(new GameplayFeatures::SoldierMovementController);
    m_Features.emplace_back(new GameplayFeatures::FactorySpawner);
    m_Features.emplace_back(new GameplayFeatures::SpaceLocation);
    m_Features.emplace_back(new GameplayFeatures::Battle);
    m_Features.emplace_back(new GameplayFeatures::HealthManagement);
    m_Features.emplace_back(new GameplayFeatures::AnimationController);
    m_Features.emplace_back(new GameplayFeatures::Sound);
}

World::~World()
{
}

void World::Update(float deltaTime, Job::JobSystem& jobSystem)
{
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