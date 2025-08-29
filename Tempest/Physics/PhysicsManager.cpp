#include <CommonIncludes.h>

#include <World/World.h>
#include <World/Components/Components.h>

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

#include <fstream>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Renderer/DebugRendererRecorder.h>

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
    JoltJobSystem()
        : JPH::JobSystemWithBarrier(1)
    {}
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

    std::ofstream DebugFile;
    JPH::StreamOutWrapper DebugStream;
    JPH::DebugRendererRecorder DebugRecorder;

    PhysicsImpl()
        : TempAllocator(10 * 1024 * 1024)
        , DebugFile(std::filesystem::path(gEngineCore->GetOptions().ResourceFolder) / "jolt_debug.jviewer", std::ios::binary)
        , DebugStream(DebugFile)
        , DebugRecorder(DebugStream)
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

void PhysicsManager::PatchWorldComponents(World& world)
{
    JPH::BodyIDVector bodies;
    m_Impl->System.GetBodies(bodies);
    JPH::BodyInterface& bodyInterface = m_Impl->System.GetBodyInterfaceNoLock();
    for (const auto& bodyId : bodies)
    {
        flecs::entity_t id = bodyInterface.GetUserData(bodyId);
        flecs::entity entity(world.m_EntityWorld, id);
        entity.get_mut<Components::PhysicsBody>().ID = bodyId;
    }
}

JPH::BodyID PhysicsManager::CreateBodyFromPrefabScene(uint32_t index, const Components::Transform& transform)
{
    JPH::BodyCreationSettings settings = m_Impl->PrefabScene->GetBodies()[index];
    settings.mPosition = ToJPH(transform.Position);
    settings.mRotation = ToJPH(transform.Rotation);
    assert(transform.Scale.x == 1.0f);
    assert(transform.Scale.y == 1.0f);
    assert(transform.Scale.z == 1.0f);

    return m_Impl->System.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
}

void PhysicsManager::CopyTransformToBody(const Components::Transform& transform, JPH::BodyID id)
{
    m_Impl->System.GetBodyInterface().SetPositionAndRotation(
        id,
        ToJPH(transform.Position),
        ToJPH(transform.Rotation),
        JPH::EActivation::Activate
    );
}

void PhysicsManager::CopyTransformFromBody(Components::Transform& transform, JPH::BodyID id)
{
    JPH::Quat rotation;
    JPH::Vec3 position;
    m_Impl->System.GetBodyInterface().GetPositionAndRotation(id, position, rotation);

    transform.Position = FromJPH(position);
    transform.Rotation = FromJPH(rotation);
}

void PhysicsManager::SetVelocity(JPH::BodyID id, glm::vec3 linear, glm::vec3 angular)
{
    m_Impl->System.GetBodyInterface().SetLinearAndAngularVelocity(id, ToJPH(linear), ToJPH(angular));
}

void PhysicsManager::RemoveBody(JPH::BodyID id)
{
    auto& interface = m_Impl->System.GetBodyInterface();
    interface.RemoveBody(id);
    interface.DestroyBody(id);
}

void PhysicsManager::Update(float deltaTime)
{
    m_Impl->System.Update(deltaTime, 1, &m_Impl->TempAllocator, &m_Impl->JobSystem);

    JPH::BodyManager::DrawSettings settings;
    m_Impl->System.DrawBodies(settings, &m_Impl->DebugRecorder);

    m_Impl->DebugRecorder.EndFrame();
}
}
