#include <CommonIncludes.h>

#include <World/World.h>

#include <World/TaskGraph/TaskGraph.h>

#include <World/Components/Components.h>
#include <World/Systems/MoveSystem.h>
#include <World/Systems/BoidsSystem.h>
#include <World/Systems/PhysicsSystem.h>
#include <World/Systems/InputControllerSystem.h>

// As we are loading the entity world form a file, we don't need to
// explicitly register components, as they would have already been registered.
template<typename ComponentType>
void RegisterComponent(ecs_world_t* world)
{
	//flecs::component<ComponentType>(world, ComponentType::Name);
	//ECS_COMPONENT(world, Type);
	ecs_component_desc_t desc = {};
	desc.entity.entity = 0;
	desc.entity.name = ComponentType::Name;
	desc.entity.symbol = ComponentType::Name;
	desc.size = sizeof(ComponentType);
	desc.alignment = alignof(ComponentType);;
	ecs_entity_t entity = ecs_component_init(world, &desc);
}

template<typename TagType>
void RegisterTag(ecs_world_t* world)
{
	//flecs::component<ComponentType>(world, ComponentType::Name);
	//ECS_COMPONENT(world, Type);
//	ECS_TAG(world, Type);
	ecs_entity_desc_t desc = {};
	desc.entity = 0;
	desc.name = TagType::Name;
	desc.symbol = TagType::Name;
	ecs_entity_t entity = ecs_entity_init(world, &desc);
}

namespace Tempest
{
World::World()
{
	m_EntityWorld = ecs_init();
	//ecs_os_api_t api;
	//ecs_os_set_api(&api);

	// Enable for debug
	//ecs_tracing_enable(3);
	ecs_set_target_fps(m_EntityWorld, 60);

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

	RegisterTag<Tags::Boids>(m_EntityWorld);
	RegisterTag<Tags::DirectionalLight>(m_EntityWorld);

	// Test Code
	//flecs::entity rect1 = flecs::entity(m_EntityWorld)
	//	.set<Components::Transform>({ glm::vec3(-0.75f, -0.75f, 0.0f) })
	//	.set<Components::Rect>({ 0.5f, 0.5f, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} });

	//flecs::entity rect2 = flecs::entity(m_EntityWorld)
	//	.set<Components::Transform>({ glm::vec3(0.5f, 0.5f, 0.0f) })
	//	.set<Components::Rect>({ 0.1f, 0.1f, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} });

	// Register all systems
	//RegisterSystem<Components::Transform>(Systems::MoveSystem::Run);
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
	ecs_fini(m_EntityWorld);
	m_EntityWorld = nullptr;
}

void World::Update(float deltaTime, Job::JobSystem& jobSystem)
{
	//m_EntityWorld.progress(deltaTime);
	{
		TaskGraph::TaskGraph graph;
		for (const auto& system : m_BeforePhysicsSystems)
		{
			system->Update(deltaTime, graph);
		}

		graph.CompileAndExecute(jobSystem);
	}

	// TODO: this probably could be fixed number and track it
	gEngine->GetPhysics().Update(deltaTime);

	{
		TaskGraph::TaskGraph graph;
		for (const auto& system : m_AfterPhysicsSystems)
		{
			system->Update(deltaTime, graph);
		}

		graph.CompileAndExecute(jobSystem);
	}
}

class MemoryInputStream
{
public:
	MemoryInputStream(const char* buffer, uint64_t bufferSize)
		: m_ReaderPosition(0)
		, m_BufferSize(bufferSize)
		, m_Buffer(buffer)
	{}

	void Read(uint8_t* bufferToFill, uint64_t bytesToRead)
	{
		assert(m_ReaderPosition + bytesToRead <= m_BufferSize);
		std::memcpy(bufferToFill, m_Buffer + m_ReaderPosition, bytesToRead);
		m_ReaderPosition += bytesToRead;
	}

	bool IsEOF() const
	{
		return m_ReaderPosition >= m_BufferSize;
	}

	template<typename T>
	friend typename std::enable_if<std::is_integral<T>::value, MemoryInputStream&>::type
		operator>>(MemoryInputStream& stream, T& value)
	{
		value = *reinterpret_cast<const T*>(stream.m_Buffer + stream.m_ReaderPosition);
		stream.m_ReaderPosition += sizeof(T);
		return stream;
	}
	friend MemoryInputStream& operator>>(MemoryInputStream& stream, eastl::string& value)
	{
		const char* stringPtr = reinterpret_cast<const char*>(stream.m_Buffer + stream.m_ReaderPosition);
		uint64_t stringLength = std::strlen(stringPtr);
		value.assign(stringPtr, stringLength);
		stream.m_ReaderPosition += stringLength + 1;
		return stream;
	}
private:
	uint64_t m_ReaderPosition;
	uint64_t m_BufferSize;
	const char* m_Buffer;
};

void World::LoadFromLevel(const char* data, size_t size)
{
	MemoryInputStream stream(data, size);

	uint32_t numArchetypes;
	stream >> numArchetypes;

	//for (uint32_t i = 0; i < numArchetypes; ++i) 
	{
		eastl::string archetypeName;
		stream >> archetypeName;

		uint32_t numEntities;
		stream >> numEntities;

		eastl::vector<Components::Transform> transforms;
		transforms.resize(numEntities);
		stream.Read(reinterpret_cast<uint8_t*>(transforms.data()), numEntities * sizeof(Components::Transform));

		eastl::vector<Components::StaticMesh> mesh;
		mesh.resize(numEntities);
		stream.Read(reinterpret_cast<uint8_t*>(mesh.data()), numEntities * sizeof(Components::StaticMesh));

		eastl::vector<Components::CarPhysicsPart> car;
		car.resize(numEntities);
		stream.Read(reinterpret_cast<uint8_t*>(car.data()), numEntities * sizeof(Components::CarPhysicsPart));

		void* pointers[] = { transforms.data(), mesh.data(), car.data() };
		ecs_bulk_desc_t desc = {};
		desc.count = numEntities;
		desc.data = pointers;
		desc.table = ecs_table_from_str(m_EntityWorld, archetypeName.c_str());

		ecs_bulk_init(m_EntityWorld, &desc);
		//break;
	}

	{
		eastl::string archetypeName;
		stream >> archetypeName;

		uint32_t numEntities;
		stream >> numEntities;

		eastl::vector<Components::Transform> transforms;
		transforms.resize(numEntities);
		stream.Read(reinterpret_cast<uint8_t*>(transforms.data()), numEntities * sizeof(Components::Transform));

		eastl::vector<Components::LightColorInfo> lightColor;
		lightColor.resize(numEntities);
		stream.Read(reinterpret_cast<uint8_t*>(lightColor.data()), numEntities * sizeof(Components::LightColorInfo));

		void* pointers[] = { transforms.data(), lightColor.data(), nullptr };
		ecs_bulk_desc_t desc = {};
		desc.count = numEntities;
		desc.data = pointers;
		desc.table = ecs_table_from_str(m_EntityWorld, archetypeName.c_str());

		ecs_bulk_init(m_EntityWorld, &desc);
		//break;
	}

	{
		eastl::string archetypeName;
		stream >> archetypeName;

		uint32_t numEntities;
		stream >> numEntities;

		eastl::vector<Components::Transform> transforms;
		transforms.resize(numEntities);
		stream.Read(reinterpret_cast<uint8_t*>(transforms.data()), numEntities * sizeof(Components::Transform));

		eastl::vector<Components::StaticMesh> mesh;
		mesh.resize(numEntities);
		stream.Read(reinterpret_cast<uint8_t*>(mesh.data()), numEntities * sizeof(Components::StaticMesh));

		void* pointers[] = { transforms.data(), mesh.data() };
		ecs_bulk_desc_t desc = {};
		desc.count = numEntities;
		desc.data = pointers;
		desc.table = ecs_table_from_str(m_EntityWorld, archetypeName.c_str());

		ecs_bulk_init(m_EntityWorld, &desc);
		//break;
	}


	//auto writer = ecs_writer_init(m_EntityWorld);
	//ecs_writer_write(data, ecs_size_t(size), &writer);

#ifdef _DEBUG
	//int i = 0;
	//while(true)
	//{
	//	auto table = ecs_dbg_get_table(m_EntityWorld.c_ptr(), i++);
	//	if (!table)
	//		break;
	//	ecs_dbg_table_t dbg_out;
	//	ecs_dbg_table(m_EntityWorld.c_ptr(), table, &dbg_out);
	//	FORMAT_LOG(Info, World, "New table with signature %s and %d num entities", ecs_type_str(m_EntityWorld.c_ptr(), dbg_out.type), dbg_out.entities_count);
	//}
#endif

	for (const auto& system : m_BeforePhysicsSystems)
	{
		system->PrepareQueries(*this);
	}

	for (const auto& system : m_AfterPhysicsSystems)
	{
		system->PrepareQueries(*this);
	}
}
}