#pragma once

// TODO: check if we can remove this from this header
#pragma warning(push)
#pragma warning(disable: 4267) // 'argument': conversion from 'size_t' to 'int32_t', possible loss of data
#include <flecs.h>
#pragma warning(pop)

namespace Tempest
{
namespace Job { class JobSystem; }
class GameplayFeature;

struct FlecsIniter
{
	FlecsIniter();
};

struct WorldStorage
{
    FlecsIniter m_Initer;
    flecs::world m_EntityWorld;

	WorldStorage();
};

class World : public WorldStorage
{
public:
	World();
	~World();

	void Update(float deltaTime, Job::JobSystem& jobSystem);

	void LoadFromLevel(const char* data, size_t size);
// TODO: maybe being private is better
//private:
	eastl::vector<eastl::unique_ptr<GameplayFeature>> m_Features;
};

uint64_t GetGlobalSeed();
}