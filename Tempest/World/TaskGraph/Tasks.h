#pragma once

#include <World/TaskGraph/TaskGraph.h>

namespace Tempest
{
namespace Task
{

struct ParallelFor : TaskGraph::Task
{
	virtual void Execute() override
	{

	}
};

struct ParallelQueryEach : TaskGraph::Task
{
	virtual void Execute() override
	{

	}

	ParallelQueryEach& SetQuery(EntitiyQuery* query)
	{
		m_Query = query;
		return *this;
	}

	ParallelQueryEach& ForEach(eastl::function<void(ecs_iter_t*)> function)
	{
		m_Function = function;
		return *this;
	}

private:
	EntitiyQuery* m_Query;
	eastl::function<void(ecs_iter_t*)> m_Function;
};

}
}