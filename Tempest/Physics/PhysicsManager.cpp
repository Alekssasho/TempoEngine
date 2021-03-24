#include <CommonIncludes.h>

#include <Physics/PhysicsManager.h>

#include <PxPhysicsAPI.h>
#include <extensions/PxExtensionsAPI.h>

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
#else
	const bool recordMemoryAllocations = false;
#endif
	m_PhysicsEngine.reset(PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, scale, recordMemoryAllocations));
	assert(m_PhysicsEngine);

	physx::PxSceneDesc sceneDesc(m_PhysicsEngine->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, 0.0f, 0.0f);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

	// TODO: Add this
	//sceneDesc.cpuDispatcher = cpuDispatcher;

	//m_Scene.reset(m_PhysicsEngine->createScene(sceneDesc));
	//assert(m_Scene);
}

PhysicsManager::~PhysicsManager()
{
}
}
