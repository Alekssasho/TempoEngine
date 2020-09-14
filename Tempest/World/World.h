#pragma once

// TODO: check if we can remove this from this header
#include <flecs.h>

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

#include <World/System.h>

namespace Tempest
{
class World
{
public:
	World();

	void Update(float deltaTime);

	void LoadFromLevel(const char* data, size_t size);
// TODO: maybe being private is better
//private:
	flecs::world m_EntityWorld;
	eastl::vector<eastl::unique_ptr<System>> m_Systems;
};
}