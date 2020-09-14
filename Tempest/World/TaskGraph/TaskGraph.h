#pragma once

#include <Utils/Graph.h>
#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

namespace Tempest
{
namespace TaskGraph
{

using TaskHandle = uint32_t;

struct Task
{
	// TODO: Look for alternatives to inheritance
	virtual void Execute() = 0;

	eastl::string Name;
	eastl::vector<TaskHandle> Dependacies;
};

class TaskGraph
{
public:
	// Executes the compiled graph using the Job System.
	// Note: You should compile the graph before executing it.
	void Execute();

	// Compile the graph tasks.
	// This will arrange all the tasks and will give
	// Order for all the task.
	// TODO: Maybe return new structure which is just the compiled info so you can have saving the graph and avoid
	// recreation every frame if nothing is changed.
	void Compile();

	template<typename TaskType>
	TaskType& CreateTask(TaskHandle& handle)
	{
		handle = TaskHandle(m_Tasks.size());
		return *static_cast<TaskType*>(m_Tasks.emplace_back(new TaskType).get());
	}
private:
	Utils::DirectedGraph<int> m_Graph;
	// TODO: remove this allocation, or put them through a special allocator
	eastl::vector<eastl::unique_ptr<Task>> m_Tasks;
};
}
}