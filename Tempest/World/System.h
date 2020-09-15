#pragma once

#include <World/EntityQuery.h>

namespace Tempest
{
namespace TaskGraph
{
class  TaskGraph;
}

// TODO: Check if we need inheritance
class System
{
public:
	virtual ~System() {}
	virtual void PrepareQueries(class World& world) = 0;
	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) = 0;
protected:
	// TODO: This should probably be more than one, but for now will do
	EntityQuery m_Query;
};
}