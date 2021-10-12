#pragma once

namespace Tempest
{
namespace TaskGraph
{
class TaskGraph;
}

// TODO: Check if we need inheritance
class System
{
public:
	virtual ~System() {}
	virtual void PrepareQueries(class World& world) = 0;
	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) = 0;
};
}