#pragma once

#include <EASTL/vector.h>

namespace Tempest
{
namespace Utils
{
template<typename T>
struct DirectedGraph
{
	struct Node
	{
		T Data;
		eastl::vector<uint16_t> Dependencies;
	};

	eastl::vector<Node> Nodes;

	void TopologicalSort()
	{
		eastl::vector<Node> sortedNodes;
		sortedNodes.reserve(Nodes.size());
		eastl::vector<bool> visitedNodes(Nodes.size(), false);

		for (auto i = 0u; i < Nodes.size(); ++i)
		{
			if (!visitedNodes[i])
			{
				VisitNode(visitedNodes, i, sortedNodes);
			}
		}

		eastl::swap(Nodes, sortedNodes);
	}

	void VisitNode(eastl::vector<bool>& visited, int index, eastl::vector<Node>& stack)
	{
		visited[index] = true;
		for (auto dependancy : Nodes[index].Dependencies)
		{
			if (!visited[dependancy])
			{
				VisitNode(visited, dependancy, stack);
			}
		}

		stack.push_back(eastl::move(Nodes[index]));
	}
};
}
}