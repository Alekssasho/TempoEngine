#include <CommonIncludes.h>

#include <Physics/PhysicsManager.h>
#include <Physics/PhysicsConstants.h>

#include <EngineCore.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Profiler.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsScene.h>

#include <DataDefinitions/PhysicsDatabase_generated.h>

#ifdef JPH_EXTERNAL_PROFILE
JPH_NAMESPACE_BEGIN
ExternalProfileMeasurement::ExternalProfileMeasurement(const char* inName, uint32 inColor)
{
    uint64_t srcLoc = ___tracy_alloc_srcloc(__LINE__, __FILE__, strlen(__FILE__), inName, strlen(inName), inColor);
    TracyCZoneCtx ctx = ___tracy_emit_zone_begin_alloc(srcLoc, true);
    memcpy(mUserData, &ctx, sizeof(TracyCZoneCtx));
}

ExternalProfileMeasurement::~ExternalProfileMeasurement()
{
    TracyCZoneCtx* ctx = reinterpret_cast<TracyCZoneCtx*>(mUserData);
    ___tracy_emit_zone_end(*ctx);
}
JPH_NAMESPACE_END
#endif

namespace Tempest
{
static void TraceImpl(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    eastl::string buffer;
    buffer.sprintf_va_list(fmt, args);
    Tempest::Logger::gLogger->WriteLog(Tempest::LogSeverity::Info, "Physics", buffer.c_str());

    va_end(args);
}

struct JoltJobSystem : public JPH::JobSystemWithBarrier
{
    //struct JoltBarrier : public JPH::JobSystem::Barrier
    //{
    //    virtual ~JoltBarrier() override
    //    {
    //        assert(Counters.empty());
    //    }
    //    virtual void		AddJob(const JobHandle& inJob) override
    //    {
    //        AddJobs(&inJob, 1);
    //    }
    //    virtual void		AddJobs(const JobHandle* inHandles, JPH::uint inNumHandles) override
    //    {
    //        for (uint32_t i = 0; i < inNumHandles; ++i)
    //        {
    //            Counters.emplace_back();
    //        }
    //    }
    //    virtual void		OnJobFinished(Job* inJob) override;

    //    eastl::vector<Tempest::Job::Counter> Counters;
    //};

    virtual int GetMaxConcurrency() const override
    {
        return gEngineCore->GetOptions().NumWorkerThreads;
    }

    virtual JobHandle		CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, JPH::uint32 inNumDependencies = 0) override
    {
        JPH::JobSystem::Job* job = new JPH::JobSystem::Job(inName, inColor, this, inJobFunction, inNumDependencies);

        if (inNumDependencies == 0)
        {
            QueueJob(job);
        }
        return JobHandle(job);
    }
    //virtual Barrier* CreateBarrier() override
    //{
    //    return new JoltBarrier;
    //}
    //virtual void			DestroyBarrier(Barrier* inBarrier) override
    //{ 
    //    JoltBarrier* jb = static_cast<JoltBarrier*>(inBarrier);
    //    delete jb;
    //}
    //virtual void			WaitForJobs(Barrier* inBarrier) override
    //{
    //    for (Tempest::Job::Counter& jobs : static_cast<JoltBarrier*>(inBarrier)->Counters)
    //    {
    //        gEngineCore->GetJobSystem().WaitForCounter(&jobs, 0);
    //    }

    //    static_cast<JoltBarrier*>(inBarrier)->Counters.resize(0);
    //}
    virtual void			QueueJob(Job* inJob) override
    {
        Tempest::Job::JobDecl job{
            [](uint32_t, void* jobData) {
                auto job = static_cast<Job*>(jobData);
                job->Execute();
            },
            inJob
        };
        gEngineCore->GetJobSystem().RunJobs("Physics Job", &job, 1);
    }
    virtual void			QueueJobs(Job** inJobs, JPH::uint inNumJobs) override
    {
        for (uint32_t i = 0; i < inNumJobs; ++i)
        {
            QueueJob(inJobs[i]);
        }
    }
    virtual void			FreeJob(Job* inJob) override
    {
        delete inJob;
    }
};

struct BPLImpl final : public JPH::BroadPhaseLayerInterface
{
    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return 2;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        if (inLayer == Physics::ObjectLayers::Static)
        {
            return Physics::BroadPhaseLayers::sStatic;
        }
        else
        {
            assert(inLayer == Physics::ObjectLayers::Dynamic);
            return Physics::BroadPhaseLayers::sDynamic;
        }
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        if (inLayer == Physics::BroadPhaseLayers::sStatic)
        {
            return "Static";
        }
        else if (inLayer == Physics::BroadPhaseLayers::sDynamic)
        {
            return "Dynamic";
        }

        return "Unknown";
    }
#endif
};

struct OBPLFImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Physics::ObjectLayers::Static:
            return inLayer2 == Physics::BroadPhaseLayers::sDynamic;
        case Physics::ObjectLayers::Dynamic:
            return true;
        default:
            assert(false);
            return false;
        }
    }
};

struct OLPFImple final : public JPH::ObjectLayerPairFilter
{
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Physics::ObjectLayers::Static:
            return inLayer2 == Physics::ObjectLayers::Dynamic;
        case Physics::ObjectLayers::Dynamic:
            return true;
        default:
            assert(false);
            return false;
        }
    }
};

struct PhysicsImpl
{
    JPH::TempAllocatorImpl TempAllocator;
    JoltJobSystem JobSystem;
    JPH::PhysicsSystem System;

    BPLImpl BPL;
    OBPLFImpl OBPLF;
    OLPFImple OLPF;

    JPH::Ref<JPH::PhysicsScene> PrefabScene;

    PhysicsImpl()
        : TempAllocator(10 * 1024 * 1024)
    {
    }
};

PhysicsManager::PhysicsManager()
{
    // TODO: override
    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
    JPH::Factory::sInstance = new JPH::Factory;
    JPH::RegisterTypes();

    m_Impl.reset(new PhysicsImpl);

    m_Impl->System.Init(
        65536,
        0,
        65536,
        10240,
        m_Impl->BPL,
        m_Impl->OBPLF,
        m_Impl->OLPF);
}

PhysicsManager::~PhysicsManager()
{
    JPH::UnregisterTypes();
    m_Impl.reset();
}

void PhysicsManager::LoadDatabase(const char* databaseName)
{
    const Definition::PhysicsDatabase* physicsDatabase = gEngineCore->GetResourceLoader().LoadResource<Definition::PhysicsDatabase>(databaseName);
    if (!physicsDatabase)
    {
        LOG(Warning, Physics, "Physics Database is Invalid!");
        return;
    }

    class StreamInFromMemory : public JPH::StreamIn
    {
    public:
        /// Constructor
        StreamInFromMemory(const uint8_t* data, uint32_t size)
            : Data(data)
            , Size(size)
            , CurrentIndex(0)
        { }
        virtual void ReadBytes(void* outData, size_t inNumBytes) override { memcpy(outData, Data + CurrentIndex, inNumBytes); CurrentIndex += uint32_t(inNumBytes); }
        virtual bool IsEOF() const override { return CurrentIndex >= Size; }
        virtual bool IsFailed() const override { return false; }

        const uint8_t* Data;
        uint32_t Size;
        uint32_t CurrentIndex;
    };

    StreamInFromMemory initialReader(physicsDatabase->initial_physics_scene()->data(), physicsDatabase->initial_physics_scene()->size());

    JPH::PhysicsScene::PhysicsSceneResult initialSceneResult = JPH::PhysicsScene::sRestoreFromBinaryState(initialReader);
    if (initialSceneResult.HasError())
    {
        FORMAT_LOG(Error, Physics, "Cannot load initial scene with %s", initialSceneResult.GetError().c_str());
        return;
    }

    JPH::Ref<JPH::PhysicsScene> initialScene = initialSceneResult.Get();
    initialScene->CreateBodies(&m_Impl->System);

    StreamInFromMemory prefabReader(physicsDatabase->prefabs_scene()->data(), physicsDatabase->prefabs_scene()->size());
    JPH::PhysicsScene::PhysicsSceneResult prefabSceneResult = JPH::PhysicsScene::sRestoreFromBinaryState(prefabReader);
    if (prefabSceneResult.HasError())
    {
        FORMAT_LOG(Error, Physics, "Cannot load prefab scene with %s", prefabSceneResult.GetError().c_str());
        return;
    }

    m_Impl->PrefabScene = prefabSceneResult.Get();
}
//
//void PhysicsManager::PatchWorldComponents(World& world, const eastl::vector<flecs::entity_t>& newlyCreatedEntities)
//{
//	physx::PxActorTypeFlags selectionFlags = physx::PxActorTypeFlag::eRIGID_DYNAMIC;
//	// TODO: Temp memory
//	eastl::vector<physx::PxActor*> actors(m_Scene->getNbActors(selectionFlags));
//	m_Scene->getActors(selectionFlags, actors.data(), physx::PxU32(actors.size()));
//
//	for (physx::PxActor* actor : actors)
//	{
//		physx::PxRigidBody* rigidBody = actor->is<physx::PxRigidBody>();
//		assert(rigidBody);
//		if (rigidBody->getNbShapes() > 1) {
//			// this is probably a car, so we handle it afterwards
//			continue;
//		}
//		flecs::entity_t id = newlyCreatedEntities[uint64_t(actor->userData)];
//		flecs::entity entity(world.m_EntityWorld, id);
//		assert(entity.has<Components::DynamicPhysicsActor>());
//		Components::DynamicPhysicsActor* dynamicActorComponent = entity.get_mut<Components::DynamicPhysicsActor>();
//		dynamicActorComponent->Actor = rigidBody;
//	}
//
//	// Setup car components
//	{
//		EntityQuery<Components::CarPhysicsPart> queryCar;
//		queryCar.Init(world);
//
//		// TODO: assert(iter.count % 5 == 0);
//		queryCar.ForEach([&actors](flecs::entity, Components::CarPhysicsPart& dynamicActor) {
//			// Find the id in userData in physics actors
//			auto findItr = eastl::find_if(actors.begin(), actors.end(), [dynamicActor](physx::PxActor* actor)
//			{
//				return size_t(actor->userData) == size_t(dynamicActor.CarActor);
//			});
//
//			assert(findItr != actors.end());
//			auto rigidBody = (*findItr)->is<physx::PxRigidDynamic>();
//			assert(rigidBody);
//			dynamicActor.CarActor = rigidBody;
//			// TODO: remove me
//			physx::PxSetGroup(*rigidBody, groupWheel);
//		});
//
//		// Setup drivable planes
//		physx::PxActorTypeFlags selectionFlagsStatic = physx::PxActorTypeFlag::eRIGID_STATIC;
//		// TODO: Temp memory
//		eastl::vector<physx::PxActor*> staticActors(m_Scene->getNbActors(selectionFlagsStatic));
//		m_Scene->getActors(selectionFlagsStatic, staticActors.data(), physx::PxU32(staticActors.size()));
//		for(const auto& staticActor : staticActors)
//		{
//			// TODO: Add simulation filtering to shapes and not actors
//			// Simulation filtering
//			physx::PxSetGroup(*staticActor, groupGround);
//		}
//	}
//}

//void PhysicsManager::Update(float deltaTime)
//{
//	if(gVehicle4W)
//	{
//		PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(gKeySmoothingData, gSteerVsForwardSpeedTable, VehicleInputData, deltaTime, gIsVehicleInAir, *gVehicle4W);
//
//		//Raycasts.
//		physx::PxVehicleWheels* vehicles[1] = { gVehicle4W };
//		PxVehicleSuspensionRaycasts(gBatchQuery, 1, vehicles, numWheels, suspensionQueryResults);
//
//		//Vehicle update.
//		const physx::PxVec3 grav = m_Scene->getGravity();
//		physx::PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];
//		physx::PxVehicleWheelQueryResult vehicleQueryResults[1] = { {wheelQueryResults, gVehicle4W->mWheelsSimData.getNbWheels()} };
//		PxVehicleUpdates(deltaTime, grav, *gFrictionPairs, 1, vehicles, vehicleQueryResults);
//
//		gIsVehicleInAir = gVehicle4W->getRigidDynamicActor()->isSleeping() ? false : physx::PxVehicleIsInAir(vehicleQueryResults[0]);
//	}
//	// TODO: Add scratch memory
//	m_Scene->simulate(deltaTime);
//
//	// TODO: Think of a way to not do it here, but do other work as well
//	m_Scene->fetchResults(true);
//
//}
}
