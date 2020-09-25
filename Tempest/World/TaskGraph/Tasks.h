#pragma once

#include <World/TaskGraph/TaskGraph.h>
#include <Job/JobSystem.h>
#include <EASTL/hash_map.h>

namespace Tempest
{
namespace Task
{

struct ParallelFor : TaskGraph::Task
{
	virtual void Execute(Job::JobSystem& jobSystem) override
	{
		// TODO:
		assert(false);
	}
};

struct ParallelQueryEach : TaskGraph::Task
{
	using Function = eastl::function<void(uint32_t, ecs_iter_t*)>;

	ParallelQueryEach(EntityQuery* query, Function function)
		: Query(query)
		, Func(function)
	{}

	virtual void Execute(Job::JobSystem& jobSystem) override
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

		Job::Counter counter;
		jobSystem.RunJobs(Name.c_str(), jobs.data(), jobCount, &counter);
		jobSystem.WaitForCounter(&counter, 0);
	}

	static void ExecuteJob(uint32_t index, void* data)
	{
		ParallelQueryEach* task = (ParallelQueryEach*)data;
		auto [count, ecsIter] = task->Query->GetIterForAchetype(index);
		task->Func(count, &ecsIter);
	}

	EntityQuery* Query;
	Function Func;
};

template<typename Key, typename Value>
struct ParallelMultiMap : TaskGraph::Task
{
	using MapType = eastl::unordered_multimap<Key, Value>;
	using Function = eastl::function<void(Value*, Value&)>;

	ParallelMultiMap(MapType& map, Function func)
		: Map(map)
		, Func(func)
	{}

	virtual void Execute(Job::JobSystem& jobSystem) override
	{
		uint32_t jobCount = uint32_t(Map.bucket_count());
		// TODO: This should be temporary memory
		eastl::vector<Job::JobDecl> jobs(jobCount);
		for (uint32_t i = 0; i < jobCount; ++i)
		{
			jobs[i].Data = (void*)this;
			jobs[i].EntryPoint = ParallelMultiMap::ExecuteJob;
		}

		Job::Counter counter;
		jobSystem.RunJobs(Name.c_str(), jobs.data(), jobCount, &counter);
		jobSystem.WaitForCounter(&counter, 0);
	}

	static void ExecuteJob(uint32_t index, void* data)
	{
		ParallelMultiMap* task = (ParallelMultiMap*)data;
		Value* firstValue = nullptr;
		for (auto itr = task->Map.begin(index); itr != task->Map.end(index); ++itr)
		{
			if (!firstValue)
			{
				firstValue = &itr->second;
				task->Func(nullptr, *firstValue);
			}
			else
			{
				task->Func(firstValue, itr->second);
			}
		}
	}

	MapType& Map;
	Function Func;
};

struct ExecuteFunction : TaskGraph::Task
{
	using Function = eastl::function<void()>;
	Function Func;
	ExecuteFunction(Function func)
		: Func(func)
	{}

	virtual void Execute(Job::JobSystem& jobSystem) override
	{
		Func();
	}
};

}
}