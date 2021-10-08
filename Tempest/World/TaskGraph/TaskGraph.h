#pragma once

#include <Utils/Graph.h>

namespace Tempest
{
namespace Job
{
class JobSystem;
struct Counter;
}
namespace TaskGraph
{

using TaskHandle = uint32_t;

struct Task
{
	// TODO: Look for alternatives to inheritance
	virtual void Execute(Job::JobSystem& jobSystem) = 0;
	void ScheduleJob(Job::JobSystem& jobSystem, Job::Counter* counter);

	eastl::string Name;
	eastl::vector<TaskHandle> Dependacies;
	class TaskGraph* Graph; // TODO: remove me
};

class TaskGraph
{
public:
	// Compile the graph tasks.
	// This will arrange all the tasks and will give
	// Order for all the task.
	// Executes the compiled graph using the Job System.
	// Note: You should compile the graph before executing it.
	void CompileAndExecute(Job::JobSystem& jobSystem);

	// TODO: This could be done for Render Graph and Build Graph but not here
	// as there are a lot of dynamic stuff and they are better handled without explicit compilation
	// TODO: Maybe return new structure which is just the compiled info so you can have saving the graph and avoid
	// recreation every frame if nothing is changed.
	//void Compile();

	template<typename TaskType>
	TaskHandle AddTask(const char* Name, TaskType* task)
	{
		TaskHandle handle = TaskHandle(m_Tasks.size());
		m_Tasks.emplace_back(task);
		m_Tasks.back()->Graph = this;
		m_Tasks.back()->Name = Name;
		return handle;
	}

	template<typename TaskType, typename... Args>
	TaskHandle CreateTask(const char* Name, Args&&... args)
	{
		return AddTask(Name, new TaskType{ eastl::forward<Args>(args)... });
	}

	void WaitFor(TaskHandle waitingTask, TaskHandle waitForTask);

	template<typename Type>
	Type* AllocateArray(uint32_t count)
	{
		m_AllocatedArrays.emplace_back(new uint8_t[count * sizeof(Type)]);
		return (Type*)m_AllocatedArrays.back().get();
	}

	static void ExecuteTaskJob(uint32_t, void*);

private:
	Utils::DirectedGraph<int> m_Graph;
	eastl::vector<int> m_ExecutionOrder;
	// TODO: remove this allocation, or put them through a special allocator
	eastl::vector<eastl::unique_ptr<Task>> m_Tasks;

	// TODO: Add allocator
	eastl::vector<eastl::unique_ptr<uint8_t>> m_AllocatedArrays;

	// TODO: very ugly please refactor me
	// This should be some kind of execution context
	eastl::vector<Job::Counter>* m_TaskCounters;
	Job::JobSystem* m_JobSystem;
};
}
}