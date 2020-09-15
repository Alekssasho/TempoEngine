#include <World/TaskGraph/TaskGraph.h>

#include <Job/JobSystem.h>

namespace Tempest
{
namespace TaskGraph
{
void TaskGraph::Execute(Job::JobSystem& jobSystem)
{
	for (int taskIndex : m_ExecutionOrder)
	{
		if (taskIndex == m_ExecutionOrder.back())
		{
			// Last element, wait for it to finish
			Job::Counter counter;
			m_Tasks[taskIndex]->Execute(jobSystem, &counter);
			jobSystem.WaitForCounter(&counter, 0);
		}
		else
		{
			m_Tasks[taskIndex]->Execute(jobSystem, nullptr);
		}
	}
}

void TaskGraph::Compile()
{
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

	// Topological Sort
	// TODO: make a topological sort
	for (auto& node : m_Graph.Nodes)
	{
		m_ExecutionOrder.push_back(node.Data);
	}
}
}
}