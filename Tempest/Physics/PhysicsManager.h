#pragma once

#include <foundation/PxErrorCallback.h>
#include <foundation/PxAllocatorCallback.h>
#include <EASTL/unique_ptr.h>

namespace physx
{
class PxFoundation;
class PxPhysics;
class PxScene;
}

namespace Tempest
{

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

template<typename T>
using PhysXPtr = eastl::unique_ptr<T, PhysXDeleter<T>>;

class PhysicsManager
{
public:
	PhysicsManager();
	~PhysicsManager();
private:
	PhysXAllocator m_Allocator;
	PhysXErrorCallback m_Logger;
	PhysXPtr<physx::PxFoundation> m_Foundation;
	PhysXPtr<physx::PxPhysics> m_PhysicsEngine;
	PhysXPtr<physx::PxScene> m_Scene;
};
}