#include <CommonIncludes.h>
#include <flecs.h>

#include <Physics/PhysicsManager.h>

#include <PxPhysicsAPI.h>
#include <extensions/PxExtensionsAPI.h>
#include <vehicle/PxVehicleSDK.h>
#include <vehicle/PxVehicleUtil.h>
#include <vehicle/PxVehicleTireFriction.h>
#include <vehicle/PxVehicleSuspLimitConstraintShader.h>


#include <World/EntityQuery.h>
#include <World/EntityQueryImpl.h>
#include <World/Components/Components.h>
#include <World/World.h>
#include <DataDefinitions/CommonTypes_generated.h>

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

physx::PxQueryHitType::Enum WheelSceneQueryPreFilterBlocking(
	physx::PxFilterData filterData0,
	physx::PxFilterData filterData1,
	const void* constantBlock,
	physx::PxU32 constantBlockSize,
	physx::PxHitFlags& queryFlags
) {
	//filterData0 is the vehicle suspension query.
	//filterData1 is the shape potentially hit by the query.
	PX_UNUSED(filterData0);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(queryFlags);
	return ((0 == (filterData1.word3 & Common::Tempest::PhysicsShapeFilter_DrivableSurface)) ? physx::PxQueryHitType::eNONE : physx::PxQueryHitType::eBLOCK);
}

const int numWheels = 4;
const int groupGround = 0;
const int groupWheel = 1;

// This should be per vehicle
physx::PxBatchQuery* gBatchQuery;
physx::PxRaycastQueryResult suspensionQueryResults[numWheels];
physx::PxRaycastHit suspensionQueryHitBuffer[numWheels];

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

	m_SerializationRegistry.reset(physx::PxSerialization::createSerializationRegistry(*m_PhysicsEngine));

	PxInitExtensions(*m_PhysicsEngine, m_VisualDebugger.get());

	// Vehicle setup
	physx::PxInitVehicleSDK(*m_PhysicsEngine, m_SerializationRegistry.get());
	physx::PxVehicleSetBasisVectors(physx::PxVec3(sUpDirection.x, sUpDirection.y, sUpDirection.z), physx::PxVec3(sForwardDirection.x, sForwardDirection.y, sForwardDirection.z));
	physx::PxVehicleSetUpdateMode(physx::PxVehicleUpdateMode::eVELOCITY_CHANGE);

	// Scene setup
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

	// Simulation filtering
	physx::PxSetGroupCollisionFlag(groupGround, groupGround, true);
	physx::PxSetGroupCollisionFlag(groupWheel, groupWheel, true);
	physx::PxSetGroupCollisionFlag(groupGround, groupWheel, false);
}

PhysicsManager::~PhysicsManager()
{
	physx::PxCloseVehicleSDK(m_SerializationRegistry.get());
	PxCloseExtensions();
}

physx::PxVehicleDrive4W* gVehicle4W = nullptr;
physx::PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs;
physx::PxVehicleTelemetryData* gTelemetryData;

void PhysicsManager::LoadFromData(void* data, uint32_t size)
{
	// we need to allocate aligned
	// Change to aligned allocator
	m_AllignedAlloc.reset(_aligned_malloc(size, 128));
	memcpy(m_AllignedAlloc.get(), data, size);

	physx::PxCollection* collection = physx::PxSerialization::createCollectionFromBinary(m_AllignedAlloc.get(), *m_SerializationRegistry);

	uint32_t numIds = collection->getNbIds();
	// TODO: Temp memory
	eastl::vector<physx::PxSerialObjectId> ids(numIds);
	collection->getIds(ids.data(), numIds);

	m_Scene->addCollection(*collection);

	// Patch userData of actors. We are using entity id as user data
	// and we have already used the same id inside the PxSerialObjectId so we need
	// to set it to the appropriate actor
	for(physx::PxSerialObjectId id : ids)
	{
		physx::PxBase* baseObject = collection->find(id);
		physx::PxActor* actor = baseObject->is<physx::PxActor>();
		physx::PxVehicleDrive4W* carDrive = nullptr;
		// Cannot use "is" method, because it is using typeMatch template which requires some internal header of the vehicle extension
		if(baseObject->getConcreteType() == physx::PxVehicleConcreteType::eVehicleDrive4W)
		{
			carDrive = static_cast<physx::PxVehicleDrive4W*>(baseObject);
		}

		if(actor)
		{
			actor->userData = (void*)id;
		}
		else if(carDrive)
		{
			assert(gVehicle4W == nullptr);
			gVehicle4W = carDrive;

			physx::PxVehicleDrivableSurfaceType surfaceTypes[1];
			surfaceTypes[0].mType = 0;

			const physx::PxMaterial* surfaceMaterials[1];
			physx::PxShape* firstShape;
			gVehicle4W->getRigidDynamicActor()->getShapes(&firstShape, 1);
			surfaceMaterials[0] = firstShape->getMaterialFromInternalFaceIndex(0);

			physx::PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs =
				physx::PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(1, 1);

			surfaceTirePairs->setup(1, 1, surfaceMaterials, surfaceTypes);

			for (int i = 0; i < 1; i++)
			{
				for (int j = 0; j < 1; j++)
				{
					surfaceTirePairs->setTypePairFriction(i, j, 1);
				}
			}
			gFrictionPairs = surfaceTirePairs;

			// 4 Wheel Vehicle setup
			physx::PxBatchQueryDesc suspensionQueryDesc(numWheels, 0, 0);
			suspensionQueryDesc.queryMemory.userRaycastResultBuffer = suspensionQueryResults;
			suspensionQueryDesc.queryMemory.userRaycastTouchBuffer = suspensionQueryHitBuffer;
			suspensionQueryDesc.queryMemory.raycastTouchBufferSize = numWheels;
			suspensionQueryDesc.preFilterShader = WheelSceneQueryPreFilterBlocking;
			gBatchQuery = m_Scene->createBatchQuery(suspensionQueryDesc);

			gVehicle4W->setToRestState();
			gVehicle4W->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eFIRST);
			gVehicle4W->mDriveDynData.setUseAutoGears(true);
			gVehicle4W->mWheelsDynData.setTireForceShaderFunction(physx::PxVehicleComputeTireForceDefault);
		}
		else
		{
			assert(false);
		}
	}

	collection->release();
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

			dynamicActor[row].Actor = rigidBody;
		}
	}

	// Setup car components
	{
		EntityQuery queryCar;
		queryCar.Init<Components::CarPhysicsPart>(world);

		for(int i = 0; i < queryCar.GetMatchedArchetypesCount(); ++i)
		{
			auto [_, iter] = queryCar.GetIterForAchetype(i);
			assert(iter.count % 5 == 0);
			Components::CarPhysicsPart* dynamicActor = ecs_column(&iter, Components::CarPhysicsPart, 1);
			for(int row = 0; row < iter.count; ++row)
			{
				// Find the id in userData in physics actors
				auto findItr = eastl::find_if(actors.begin(), actors.end(), [dynamicActor, row](physx::PxActor* actor)
				{
					return size_t(actor->userData) == size_t(dynamicActor[row].CarActor);
				});

				assert(findItr != actors.end());
				auto rigidBody = (*findItr)->is<physx::PxRigidDynamic>();
				assert(rigidBody);
				dynamicActor[row].CarActor = rigidBody;
				// TODO: remove me
				physx::PxSetGroup(*rigidBody, groupWheel);
			}
		}

		// Setup drivable planes
		physx::PxActorTypeFlags selectionFlagsStatic = physx::PxActorTypeFlag::eRIGID_STATIC;
		// TODO: Temp memory
		eastl::vector<physx::PxActor*> staticActors(m_Scene->getNbActors(selectionFlagsStatic));
		m_Scene->getActors(selectionFlagsStatic, staticActors.data(), physx::PxU32(staticActors.size()));
		for(const auto& staticActor : staticActors)
		{
			// TODO: Add simulation filtering to shapes and not actors
			// Simulation filtering
			physx::PxSetGroup(*staticActor, groupGround);
		}
	}
}

physx::PxVehicleKeySmoothingData gKeySmoothingData =
{
	{
		6.0f,	//rise rate eANALOG_INPUT_ACCEL
		6.0f,	//rise rate eANALOG_INPUT_BRAKE
		6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE
		2.5f,	//rise rate eANALOG_INPUT_STEER_LEFT
		2.5f,	//rise rate eANALOG_INPUT_STEER_RIGHT
	},
	{
		10.0f,	//fall rate eANALOG_INPUT_ACCEL
		10.0f,	//fall rate eANALOG_INPUT_BRAKE
		10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE
		5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT
		5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT
	}
};

physx::PxF32 gSteerVsForwardSpeedData[2 * 8] =
{
	0.0f,		0.75f,
	5.0f,		0.75f,
	30.0f,		0.125f,
	120.0f,		0.1f,
	PX_MAX_F32, PX_MAX_F32,
	PX_MAX_F32, PX_MAX_F32,
	PX_MAX_F32, PX_MAX_F32,
	PX_MAX_F32, PX_MAX_F32
};
physx::PxFixedSizeLookupTable<8> gSteerVsForwardSpeedTable(gSteerVsForwardSpeedData, 4);
bool gIsVehicleInAir = true;

void PhysicsManager::Update(float deltaTime)
{
	if(gVehicle4W)
	{
		PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(gKeySmoothingData, gSteerVsForwardSpeedTable, VehicleInputData, deltaTime, gIsVehicleInAir, *gVehicle4W);

		//Raycasts.
		physx::PxVehicleWheels* vehicles[1] = { gVehicle4W };
		PxVehicleSuspensionRaycasts(gBatchQuery, 1, vehicles, numWheels, suspensionQueryResults);

		//Vehicle update.
		const physx::PxVec3 grav = m_Scene->getGravity();
		physx::PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];
		physx::PxVehicleWheelQueryResult vehicleQueryResults[1] = { {wheelQueryResults, gVehicle4W->mWheelsSimData.getNbWheels()} };
		PxVehicleUpdates(deltaTime, grav, *gFrictionPairs, 1, vehicles, vehicleQueryResults);

		gIsVehicleInAir = gVehicle4W->getRigidDynamicActor()->isSleeping() ? false : physx::PxVehicleIsInAir(vehicleQueryResults[0]);
	}
	// TODO: Add scratch memory
	m_Scene->simulate(deltaTime);

	// TODO: Think of a way to not do it here, but do other work as well
	m_Scene->fetchResults(true);

}
}
