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
};
}
}