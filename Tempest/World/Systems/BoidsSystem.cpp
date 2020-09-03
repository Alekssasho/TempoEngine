#include <World/Systems/BoidsSystem.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

namespace Tempest
{
namespace Systems
{
void BoidsSystem::Run(flecs::world& world)
{
	//flecs::filter filter = flecs::filter(world)
	//	.include<Components::Transform>()
	//	.include_kind(flecs::MatchAll);

	int totalCount = 0;
	flecs::query query = flecs::query<Components::Transform>(world);
	query.action([&totalCount](flecs::iter& it, flecs::column<Components::Transform> t) {
		totalCount += it.count();
	});

	//TODO: Use temporary memory
	eastl::vector<int> cellIndices(totalCount);
	eastl::vector<int> cellTargetPositionIndex(totalCount);
	eastl::vector<int> cellCount(totalCount);
	eastl::vector<glm::vec3> cellAlignment(totalCount);
	eastl::vector<glm::vec3> cellSeparation(totalCount);
	eastl::unordered_multimap<int, int> hashMap(totalCount);

	// InitialCellAlignmentJob
	int i = 0;
	query.each([&cellAlignment, &cellSeparation, &i](flecs::entity e, Components::Transform& transform) {
		cellAlignment[i] = transform.Heading;
		cellSeparation[i] = transform.Position;
		++i;
	});


}
}
}