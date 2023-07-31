#include <CommonIncludes.h>

#include <World/World.h>

#include <World/TaskGraph/TaskGraph.h>

#include <World/Components/Components.h>
#include <World/Systems/MoveSystem.h>
#include <World/Systems/BoidsSystem.h>
#include <World/Systems/PhysicsSystem.h>
#include <World/Systems/InputControllerSystem.h>

template<typename ComponentType>
void RegisterComponent(flecs::world& world)
{
	flecs::component<ComponentType>(world, ComponentType::Name);
}

namespace Tempest
{
World::World()
{
	//ecs_os_api_t api;
	//ecs_os_set_api(&api);

	// Enable for debug
	//ecs_tracing_enable(3);
	m_EntityWorld.set_target_fps(60);

	m_EntityWorld.set_stage_count(2);

	// This is not needed when we are loading the world from serialized data
	// Register all components
	RegisterComponent<Components::Transform>(m_EntityWorld);
	RegisterComponent<Components::Rect>(m_EntityWorld);
	RegisterComponent<Components::StaticMesh>(m_EntityWorld);
	RegisterComponent<Components::CameraController>(m_EntityWorld);
	RegisterComponent<Components::CarPhysicsPart>(m_EntityWorld);
	RegisterComponent<Components::DynamicPhysicsActor>(m_EntityWorld);
	RegisterComponent<Components::LightColorInfo>(m_EntityWorld);
	RegisterComponent<Components::VehicleController>(m_EntityWorld);

	RegisterComponent<Tags::Boids>(m_EntityWorld);
	RegisterComponent<Tags::DirectionalLight>(m_EntityWorld);

	// Register all systems
	//m_Systems.emplace_back(new Systems::MoveSystem);
	//m_Systems.emplace_back(new Systems::BoidsSystem);

	// This should be the last system in this bucket
	// TODO: Mirror To Physics is needed only for kinematics objects as they are driven by animation, everything else should be physics driven
	//m_BeforePhysicsSystems.emplace_back(new Systems::MirrorToPhysics);
	m_BeforePhysicsSystems.emplace_back(new Systems::CameraControllerSystem);
	m_BeforePhysicsSystems.emplace_back(new Systems::VehicleControllerSystem);

	// This should be the first system in this bucket
	m_AfterPhysicsSystems.emplace_back(new Systems::MirrorFromPhysics);
}

World::~World()
{
	m_BeforePhysicsSystems.clear();
	m_AfterPhysicsSystems.clear();
}

void World::Update(float deltaTime, Job::JobSystem& jobSystem)
{
	//m_EntityWorld.progress(deltaTime);
	{
		m_EntityWorld.readonly_begin();
		TaskGraph::TaskGraph graph;
		for (const auto& system : m_BeforePhysicsSystems)
		{
			system->Update(deltaTime, graph);
		}

		graph.CompileAndExecute(jobSystem);
		m_EntityWorld.readonly_end();
	}

	// TODO: this probably could be fixed number and track it
	gEngine->GetPhysics().Update(deltaTime);

	{
		m_EntityWorld.readonly_begin();
		TaskGraph::TaskGraph graph;
		for (const auto& system : m_AfterPhysicsSystems)
		{
			system->Update(deltaTime, graph);
		}

		graph.CompileAndExecute(jobSystem);
		m_EntityWorld.readonly_end();
	}
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
	eastl::vector<flecs::entity_t> newlyCreatedEntityIds;

	MemoryInputStream stream(reinterpret_cast<const uint8_t*>(data), size);

	uint32_t numArchetypes;
	stream.Read(numArchetypes);

	for (uint32_t i = 0; i < numArchetypes; ++i) {
		uint32_t numEntities;
		stream.Read(numEntities);

		// Read the number of components
		uint32_t numComponents;
		stream.Read(numComponents);

		// TODO: Temp memory
		eastl::vector<uint32_t> componentSizes;
		componentSizes.resize(numComponents);
		stream.Read(reinterpret_cast<uint8_t*>(componentSizes.data()), numComponents * sizeof(uint32_t));

		uint32_t nameLen;
		stream.Read(nameLen);

		auto archetypeName = reinterpret_cast<const char*>(stream.Jump(nameLen));

		eastl::vector<const void*> componentArrayPointers;
		componentArrayPointers.reserve(numComponents);

		for (auto size : componentSizes) {
			// This is a tag, so we just push a nullptr
			if (size == 0) {
				componentArrayPointers.push_back(nullptr);
			}
			else
			{
				componentArrayPointers.push_back(stream.Jump(numEntities * size));
			}
		}

		const uint32_t beforeSize = uint32_t(newlyCreatedEntityIds.size());
		newlyCreatedEntityIds.resize(beforeSize + numEntities);

		ecs_bulk_desc_t desc = {};
		desc.count = numEntities;
		desc.data = const_cast<void**>(componentArrayPointers.data());
		//desc.table = ecs_table_from_str(m_EntityWorld, archetypeName);

		uint32_t currentTermCount = 0;
		ecs_term_t currentTerm;
		while (archetypeName[0] && (archetypeName = ecs_parse_term(m_EntityWorld, nullptr, nullptr, archetypeName, &currentTerm)))
		{
			ecs_term_finalize(m_EntityWorld, &currentTerm);
			desc.ids[currentTermCount] = currentTerm.id;
			++currentTermCount;
			ecs_term_fini(&currentTerm);
		}
		//desc.table = ecs_table_find(m_EntityWorld, )

		const ecs_entity_t* newlyCreatedIds = ecs_bulk_init(m_EntityWorld, &desc);
		memcpy(newlyCreatedEntityIds.data() + beforeSize, newlyCreatedIds, numEntities * sizeof(ecs_entity_t));
	}

	for (const auto& system : m_BeforePhysicsSystems)
	{
		system->PrepareQueries(*this);
	}

	for (const auto& system : m_AfterPhysicsSystems)
	{
		system->PrepareQueries(*this);
	}

	return newlyCreatedEntityIds;
}
}