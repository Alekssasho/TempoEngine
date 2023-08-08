#pragma once

// TODO: check if we can remove this from this header
#include <flecs.h>

namespace Tempest
{
namespace Job { class JobSystem; }
class GameplayFeature;

struct FlecsIniter
{
	FlecsIniter();
};

class World
{
public:
	World();
	~World();

	void Update(float deltaTime, Job::JobSystem& jobSystem);

	eastl::vector<flecs::entity_t> LoadFromLevel(const char* data, size_t size);
// TODO: maybe being private is better
//private:

	FlecsIniter m_Initer;
	flecs::world m_EntityWorld;

	eastl::vector<eastl::unique_ptr<GameplayFeature>> m_Features;
};
}