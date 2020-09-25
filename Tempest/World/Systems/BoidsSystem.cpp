#include <World/Systems/BoidsSystem.h>
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

	int* cellIndices = graph.AllocateArray<int>(totalCount);
	int* cellTargetPositionIndex = graph.AllocateArray<int>(totalCount);
	int* cellCount = graph.AllocateArray<int>(totalCount);
	glm::vec3* cellAlignment = graph.AllocateArray<glm::vec3>(totalCount);
	glm::vec3* cellSeparation = graph.AllocateArray<glm::vec3>(totalCount);

	// TODO: This could probably better
	eastl::unordered_multimap<int, int>* hashMap = new eastl::unordered_multimap<int, int>;

	TaskGraph::TaskHandle deleteHashMap = graph.CreateTask<Task::ExecuteFunction>(
		"BoidsSystem::DeleteHashMap",
		[hashMap] {
			delete hashMap;
		});

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

	TaskGraph::TaskHandle initializeDataJob = graph.CreateTask<Task::ParallelQueryEach>(
		"BoidsSystem::InitializeData",
		&m_Query,
		[cellAlignment, cellSeparation, cellCount, hashMap, settings](uint32_t index, ecs_iter_t* iter) {
			Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
			for (int i = 0; i < iter->count; ++i)
			{
				cellAlignment[index] = transform[i].Heading; // InitialCellAlignmentJob
				cellSeparation[index] = transform[i].Position; // Initial Cell Separation Job
				cellCount[index] = 1; // Initial Cell Count Job

				// Populate Hash Map Job
				std::hash<glm::ivec3> hasher;
				int hash = int(hasher(glm::ivec3(glm::floor((transform[i].Position * 100.0f) / settings.cellRadius))));
				//TODO: this is not thread safe
				hashMap->insert(eastl::make_pair(hash, index));

				// TODO: For now we need to do it by hand due to looping over archetypes
				++index;
			}
		});

	// Merge Cells Jobs
	TaskGraph::TaskHandle mergeCellJob = graph.CreateTask<Task::ParallelMultiMap<int, int>>(
		"BoidsSystem::MergeCells",
		*hashMap,
		[cellIndices, cellTargetPositionIndex, cellAlignment, cellSeparation, cellCount, hashMap](int* first, int& value) {
			if (!first)
			{
				cellTargetPositionIndex[value] = 0; // We have a single target for now

				cellIndices[value] = value;
			}
			else
			{
				cellCount[*first] += 1;
				cellAlignment[*first] = cellAlignment[*first] + cellAlignment[value];
				cellSeparation[*first] = cellSeparation[*first] + cellSeparation[value];
				cellIndices[value] = *first;

			}
		});

	// Steer Job
	TaskGraph::TaskHandle steerJobHandle = graph.CreateTask<Task::ParallelQueryEach>(
		"BoidsSystem::Steer",
		&m_Query,
		[cellAlignment, cellSeparation, cellCount, cellIndices, cellTargetPositionIndex, settings, deltaTime](uint32_t index, ecs_iter_t* iter) {
			Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
			for (int i = 0; i < iter->count; ++i)
			{
				// temporarily storing the values for code readability
				glm::vec3 forward = transform[i].Heading;
				glm::vec3 currentPosition = transform[i].Position;
				int cellIndex = cellIndices[index];
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

				++index;
			}
		});

	graph.WaitFor(mergeCellJob, initializeDataJob);
	graph.WaitFor(steerJobHandle, mergeCellJob);
	graph.WaitFor(deleteHashMap, steerJobHandle);
}
}
}