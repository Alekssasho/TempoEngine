#pragma once

#include <foundation/PxErrorCallback.h>
#include <foundation/PxAllocatorCallback.h>
#include <pvd/PxPvd.h>

// TODO: remove
#include <vehicle/PxVehicleUtilControl.h>

namespace physx
{
class PxFoundation;
class PxPhysics;
class PxScene;
}

namespace Tempest
{
	class World;

	struct PhysXErrorCallback : physx::PxErrorCallback
{
	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
};

struct PhysXAllocator : physx::PxAllocatorCallback
{
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override;

	virtual void deallocate(void* ptr) override;
};

template<typename T>
struct PhysXDeleter
{
	void operator()(T* physxObj)
	{
		physxObj->release();
	}
};

struct AllignedDeleter
{
	void operator()(void* ptr)
	{
		_aligned_free(ptr);
	}
};

template<typename T>
using PhysXPtr = eastl::unique_ptr<T, PhysXDeleter<T>>;

class PhysicsManager
{
public:
	PhysicsManager();
	~PhysicsManager();

	void LoadFromData(void* data, uint32_t size);
	void PatchWorldComponents(World& world);
	void Update(float deltaTime);
private:
	eastl::unique_ptr<void, AllignedDeleter> m_AllignedAlloc;
	PhysXAllocator m_Allocator;
	PhysXErrorCallback m_Logger;
	PhysXPtr<physx::PxFoundation> m_Foundation;
	PhysXPtr<physx::PxPvdTransport> m_Transport;
	PhysXPtr<physx::PxPvd> m_VisualDebugger;
	PhysXPtr<physx::PxPhysics> m_PhysicsEngine;
	PhysXPtr<physx::PxScene> m_Scene;

public:
	// TODO: remove
	physx::PxVehicleDrive4WRawInputData VehicleInputData;
};
}