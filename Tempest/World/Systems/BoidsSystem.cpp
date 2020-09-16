#include <World/Systems/BoidsSystem.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

#include <World/TaskGraph/Tasks.h>
#include <World/EntityQueryImpl.h>

namespace Tempest
{
namespace Systems
{

glm::vec3 normalize_safe(glm::vec3 input)
{
	float squaredLen = glm::dot(input, input);
	if (squaredLen > 1e-6f)
	{
		return input / glm::sqrt(squaredLen);
	}
	else
	{
		return glm::vec3();
	}
}

void BoidsSystem::PrepareQueries(World& world)
{
	m_Query.Init<Components::Transform>(world);
}

void BoidsSystem::Update(float deltaTime, TaskGraph::TaskGraph& graph)
{
	int totalCount = m_Query.GetMatchedEntitiesCount();

	//TODO: Use temporary memory
	eastl::vector<int> cellIndices(totalCount);
	eastl::vector<int> cellTargetPositionIndex(totalCount);
	eastl::vector<int> cellCount(totalCount);
	eastl::vector<glm::vec3> cellAlignment(totalCount);
	eastl::vector<glm::vec3> cellSeparation(totalCount);
	eastl::unordered_multimap<int, int> hashMap(totalCount);

	struct BoidsSettings {
		float cellRadius;
		glm::vec3 TargetPosition;
		float SeparationWeight;
		float AlignmentWeight;
		float TargetWeight;
		float MoveSpeed;
	} settings{
		5.0f,
		glm::vec3(0.0f),
		1.0f,
		1.0f,
		2.0f,
		0.1f
	};

	int entitiyIndex = 0;
	// TODO: Pass entityIndex from inside the task. As this is called multitreaded it will break the entityIndex
	TaskGraph::TaskHandle initializeDataJob = graph.CreateTask<Task::ParallelQueryEach>(&m_Query, [&cellAlignment, &cellSeparation, &cellCount, &hashMap, &settings, &entitiyIndex](ecs_iter_t* iter) {
		Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
		for (int i = 0; i < iter->count; ++i)
		{
			cellAlignment[entitiyIndex] = transform[i].Heading; // InitialCellAlignmentJob
			cellSeparation[entitiyIndex] = transform[i].Position; // Initial Cell Separation Job
			cellCount[entitiyIndex] = 1; // Initial Cell Count Job

			// Populate Hash Map Job
			std::hash<glm::ivec3> hasher;
			int hash = int(hasher(glm::ivec3(glm::floor((transform[i].Position * 100.0f) / settings.cellRadius))));
			hashMap.insert(eastl::make_pair(hash, entitiyIndex));

			++entitiyIndex;
		}
	});

	// Merge Cells Jobs
	for (int i = 0; i < hashMap.bucket_count(); ++i)
	{
		bool first = true;
		int firstIndex = 0;
		for (auto itr = hashMap.begin(i); itr != hashMap.end(i); ++itr)
		{
			int index = itr->second;
			if (first)
			{
				firstIndex = index;

				cellTargetPositionIndex[index] = 0; // We have a single target for now

				cellIndices[index] = index;
				first = false;
			}
			else
			{
				cellCount[firstIndex] += 1;
				cellAlignment[firstIndex] = cellAlignment[firstIndex] + cellAlignment[index];
				cellSeparation[firstIndex] = cellSeparation[firstIndex] + cellSeparation[index];
				cellIndices[index] = firstIndex;
			}
		}
	}

	// Steer Job
	entitiyIndex = 0;
	TaskGraph::TaskHandle steerJobHandle = graph.CreateTask<Task::ParallelQueryEach>(&m_Query, [&cellAlignment, &cellSeparation, &cellCount, &cellIndices, &cellTargetPositionIndex, &settings, &deltaTime, &entitiyIndex](ecs_iter_t* iter) {
		Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
		for (int i = 0; i < iter->count; ++i)
		{
			// temporarily storing the values for code readability
			glm::vec3 forward = transform[i].Heading;
			glm::vec3 currentPosition = transform[i].Position;
			int cellIndex = cellIndices[entitiyIndex];
			int neighborCount = cellCount[cellIndex];
			glm::vec3 alignment = cellAlignment[cellIndex];
			glm::vec3 separation = cellSeparation[cellIndex];
			int nearestTargetPositionIndex = cellTargetPositionIndex[cellIndex];
			glm::vec3 nearestTargetPosition = settings.TargetPosition;

			// Setting up the directions for the three main biocrowds influencing directions adjusted based
			// on the predefined weights:
			// 1) alignment - how much should it move in a direction similar to those around it?
			// note: we use `alignment/neighborCount`, because we need the average alignment in this case; however
			// alignment is currently the summation of all those of the boids within the cellIndex being considered.
			glm::vec3 alignmentResult = settings.AlignmentWeight
				* normalize_safe((alignment / float(neighborCount)) - forward);
			// 2) separation - how close is it to other boids and are there too many or too few for comfort?
			// note: here separation represents the summed possible center of the cell. We perform the multiplication
			// so that both `currentPosition` and `separation` are weighted to represent the cell as a whole and not
			// the current individual boid.
			glm::vec3 separationResult = settings.SeparationWeight
				* normalize_safe((currentPosition * float(neighborCount)) - separation);
			// 3) target - is it still towards its destination?
			glm::vec3 targetHeading = settings.TargetWeight
				* normalize_safe(nearestTargetPosition - currentPosition);


			glm::vec3 targetForward = normalize_safe(alignmentResult + separationResult + targetHeading);

			// updates using the newly calculated heading direction
			glm::vec3 nextHeading = normalize_safe(forward + deltaTime * (targetForward - forward));

			transform[i].Heading = nextHeading;
			transform[i].Position = transform[i].Position + (nextHeading * settings.MoveSpeed * deltaTime);

			++entitiyIndex;
		}
	});
}
}
}