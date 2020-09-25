#include <World/TaskGraph/TaskGraph.h>

#include <Job/JobSystem.h>

namespace Tempest
{
namespace TaskGraph
{
void TaskGraph::CompileAndExecute(Job::JobSystem& jobSystem)
{
	m_JobSystem = &jobSystem;

	for (int i = 0; i < m_Tasks.size(); ++i)
	{
		Utils::DirectedGraph<int>::Node node;
		node.Data = i;
		for (TaskHandle dependancy : m_Tasks[i]->Dependacies)
		{
			node.Dependencies.push_back(dependancy);
		}
		m_Graph.Nodes.push_back(node);
	}

	m_Graph.TopologicalSort();

	for (auto& node : m_Graph.Nodes)
	{
		m_ExecutionOrder.push_back(node.Data);
	}

	eastl::vector<Job::Counter> jobCounters(m_ExecutionOrder.size());
	m_TaskCounters = &jobCounters;
	for (int taskIndex : m_ExecutionOrder)
	{
		m_Tasks[taskIndex]->ScheduleJob(jobSystem, &jobCounters[taskIndex]);
	}

	// Wait for all the counters
	for (auto& counter : jobCounters)
	{
		jobSystem.WaitForCounter(&counter, 0);
	}
}

void TaskGraph::WaitFor(TaskHandle waitingTask, TaskHandle waitForTask)
{
	m_Tasks[waitingTask]->Dependacies.push_back(waitForTask);
}

void TaskGraph::ExecuteTaskJob(uint32_t, void* data)
{
	Task* task = (Task*)data;
	// First wait for dependancies
	for (auto dependancy : task->Dependacies)
	{
		// TODO: This is very ugly, find a way to clean it up.
		task->Graph->m_JobSystem->WaitForCounter(&(*task->Graph->m_TaskCounters)[dependancy], 0);
	}

	task->Execute(*task->Graph->m_JobSystem);
}

void Task::ScheduleJob(Job::JobSystem& jobSystem, Job::Counter* counter)
{
	Job::JobDecl job;
	job.Data = (void*)this;
	job.EntryPoint = TaskGraph::ExecuteTaskJob;

	jobSystem.RunJobs("Wait For Dependacies Task", &job, 1, counter);
}
}
}