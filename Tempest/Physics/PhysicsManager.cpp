#include <CommonIncludes.h>
#include <flecs.h>

#include <Physics/PhysicsManager.h>

#include <PxPhysicsAPI.h>
#include <extensions/PxExtensionsAPI.h>
#include <World/EntityQuery.h>
#include <World/EntityQueryImpl.h>
#include <World/Components/Components.h>
#include <World/World.h>

namespace Tempest
{
void PhysXErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
{
	Tempest::LogSeverity severity;
	switch (code)
	{
	case physx::PxErrorCode::eDEBUG_INFO:
		severity = Tempest::LogSeverity::Info;
		break;
	case physx::PxErrorCode::eDEBUG_WARNING:
	case physx::PxErrorCode::ePERF_WARNING:
		severity = Tempest::LogSeverity::Warning;
		break;
	case physx::PxErrorCode::eINVALID_PARAMETER:
	case physx::PxErrorCode::eINVALID_OPERATION:
	case physx::PxErrorCode::eOUT_OF_MEMORY:
	case physx::PxErrorCode::eINTERNAL_ERROR:
		severity = Tempest::LogSeverity::Error;
		break;
	case physx::PxErrorCode::eABORT:
	default:
		severity = Tempest::LogSeverity::Fatal;
		break;
	}
	
	Tempest::Logger::gLogger->WriteLog(severity, "Physics", message);
}

void* PhysXAllocator::allocate(size_t size, const char* typeName, const char* filename, int line)
{
	// TODO: Proper allocator
	return malloc(size);
}

void PhysXAllocator::deallocate(void* ptr)
{
	free(ptr);
}

PhysicsManager::PhysicsManager()
{
	m_Foundation.reset(PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_Logger));
	assert(m_Foundation);

	physx::PxTolerancesScale scale;

#ifdef _DEBUG
	const bool recordMemoryAllocations = true;
	//The normal way to connect to pvd.  PVD needs to be running at the time this function is called.
	//We don't worry about the return value because we are already registered as a listener for connections
	//and thus our onPvdConnected call will take care of setting up our basic connection state.
	char ip[] = "127.0.0.1";
	m_Transport.reset(physx::PxDefaultPvdSocketTransportCreate(ip, 5425, 1000));
	if (!m_Transport)
	{
		return;
	}

	//Use these flags for a clean profile trace with minimal overhead
	physx::PxPvdInstrumentationFlags flags = physx::PxPvdInstrumentationFlag::eALL;

	m_VisualDebugger.reset(physx::PxCreatePvd(*m_Foundation));
	m_VisualDebugger->connect(*m_Transport, flags);
#else
	const bool recordMemoryAllocations = false;
#endif
	m_PhysicsEngine.reset(PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, scale, recordMemoryAllocations, m_VisualDebugger.get()));
	assert(m_PhysicsEngine);

	PxInitExtensions(*m_PhysicsEngine, m_VisualDebugger.get());

	physx::PxSceneDesc sceneDesc(m_PhysicsEngine->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.8f, 0.0f);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

	// TODO: Change to one specific for our job system
	sceneDesc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);

	m_Scene.reset(m_PhysicsEngine->createScene(sceneDesc));
	assert(m_Scene);

	m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 1.f);
	m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eACTOR_AXES, 2.f);
	m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);

	physx::PxPvdSceneClient* pvdClient = m_Scene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
}

PhysicsManager::~PhysicsManager()
{
	PxCloseExtensions();
}

void PhysicsManager::LoadFromData(void* data, uint32_t size)
{
	// we need to allocate aligned
	// Change to aligned allocator
	m_AllignedAlloc.reset(_aligned_malloc(size, 128));
	memcpy(m_AllignedAlloc.get(), data, size);
	
	physx::PxSerializationRegistry* registry = physx::PxSerialization::createSerializationRegistry(*m_PhysicsEngine);
	
	physx::PxCollection* collection = physx::PxSerialization::createCollectionFromBinary(m_AllignedAlloc.get(), *registry);

	uint32_t numIds = collection->getNbIds();
	// TODO: Temp memory
	eastl::vector<physx::PxSerialObjectId> ids(numIds);
	collection->getIds(ids.data(), numIds);

	// Patch userData of actors. We are using entity id as user data
	// and we have already used the same id inside the PxSerialObjectId so we need
	// to set it to the appropriate actor
	for(physx::PxSerialObjectId id : ids)
	{
		physx::PxActor* actor = collection->find(id)->is<physx::PxActor>();
		if(actor)
		{
			actor->userData = (void*)id;
		}
	}

	m_Scene->addCollection(*collection);

	collection->release();
	registry->release();
}

void PhysicsManager::PatchWorldComponents(World& world)
{
	physx::PxActorTypeFlags selectionFlags = physx::PxActorTypeFlag::eRIGID_DYNAMIC;
	// TODO: Temp memory
	eastl::vector<physx::PxActor*> actors(m_Scene->getNbActors(selectionFlags));
	m_Scene->getActors(selectionFlags, actors.data(), physx::PxU32(actors.size()));

	// TODO: this is very inefficient, but currently we cannot set
	// a component to a entity directly due to missing components id
	// Consider changing this to something better
	EntityQuery query;
	query.Init<Components::DynamicPhysicsActor>(world);

	int archetypeCount = query.GetMatchedArchetypesCount();
	for (int i = 0; i < archetypeCount; ++i)
	{
		auto [_, iter] = query.GetIterForAchetype(i);
		Components::DynamicPhysicsActor* dynamicActor = ecs_column(&iter, Components::DynamicPhysicsActor, 1);
		for (int row = 0; row < iter.count; ++row)
		{
			ecs_entity_t id = iter.entities[row];
			// Find the id in userData in physics actors
			auto findItr = eastl::find_if(actors.begin(), actors.end(), [id](physx::PxActor* actor)
			{
				return ecs_entity_t(actor->userData) == id;
			});

			assert(findItr != actors.end());
			auto rigidBody = (*findItr)->is<physx::PxRigidBody>();
			assert(rigidBody);

			dynamicActor->Actor = rigidBody;
		}
	}
}

void PhysicsManager::Update(float deltaTime)
{
	// TODO: Add scratch memory
	m_Scene->simulate(deltaTime);

	// TODO: Think of a way to not do it here, but do other work as well
	m_Scene->fetchResults(true);
}
}