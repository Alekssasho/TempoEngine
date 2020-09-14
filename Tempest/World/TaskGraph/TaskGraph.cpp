#include <World/TaskGraph/TaskGraph.h>

namespace Tempest
{
namespace TaskGraph
{
void TaskGraph::Execute()
{

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
}
}
}