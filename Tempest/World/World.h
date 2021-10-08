#pragma once

// TODO: check if we can remove this from this header
#include <flecs.h>

#include <World/System.h>

namespace Tempest
{
namespace Job { class JobSystem; }

class World
{
public:
	World();
	~World();

	void Update(float deltaTime, Job::JobSystem& jobSystem);

	void LoadFromLevel(const char* data, size_t size);
// TODO: maybe being private is better
//private:
	flecs::world m_EntityWorld;
	eastl::vector<eastl::unique_ptr<System>> m_BeforePhysicsSystems;
	eastl::vector<eastl::unique_ptr<System>> m_AfterPhysicsSystems;
};
}