#pragma once

#include <World/TaskGraph/TaskGraph.h>
#include <Job/JobSystem.h>

namespace Tempest
{
namespace Task
{

struct ParallelFor : TaskGraph::Task
{
	virtual void Execute(Job::JobSystem& jobSystem, Job::Counter* counter) override
	{
		// TODO:
		assert(false);
	}
};

struct ParallelQueryEach : TaskGraph::Task
{
	ParallelQueryEach(EntityQuery* query, eastl::function<void(ecs_iter_t*)> function)
		: Query(query)
		, Function(function)
	{}

	virtual void Execute(Job::JobSystem& jobSystem, Job::Counter* counter) override
	{
		assert(Query);
		int jobCount = Query->GetMatchedArchetypesCount();
		// TODO: This should be temporary memory
		eastl::vector<Job::JobDecl> jobs(jobCount);
		for (int i = 0; i < jobCount; ++i)
		{
			jobs[i].Data = (void*)this;
			jobs[i].EntryPoint = ParallelQueryEach::ExecuteJob;
		}

		jobSystem.RunJobs(Name.c_str(), jobs.data(), jobCount, counter);
	}

	static void ExecuteJob(uint32_t index, void* data)
	{
		ParallelQueryEach* task = (ParallelQueryEach*)data;
		auto ecsIter = task->Query->GetIterForAchetype(index);
		task->Function(&ecsIter);
	}

	EntityQuery* Query;
	eastl::function<void(ecs_iter_t*)> Function;
};

}
}