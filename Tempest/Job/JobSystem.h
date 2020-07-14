#pragma once

#include <Defines.h>

#include <atomic>
#include <thread>
#include <mutex>

#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

#include <Job/Queue.h>

namespace Tempest
{
namespace Job
{

using JobEntryPoint = void(*)(void*);

struct JobDecl
{
	JobEntryPoint EntryPoint;
	void* Data;
};

// NB: Compile with Enable Fiber-Safe Optimizations

// This is public to allow stack allocation
// Do not modify or set Value. The JobSystem will use it
struct Counter
{
	std::atomic<unsigned> Value = 0u;
};

using FiberHandle = void*;

class TEMPEST_API JobSystem
{
public:
	JobSystem(uint32_t numWorkerThreads, uint32_t numFibers, uint32_t fiberStackSize);
	~JobSystem();

	// Can be called from anywhere
	void RunJobs(const char* name, JobDecl* jobs, uint32_t numJobs, Counter* counter = nullptr);

	// Note only 1 job can wait on some counter. This sounds reasonable for now. Implement if ever needed.
	// Can be called only from a Job
	void WaitForCounter(Counter* counter, uint32_t value);

	// Will set internal flag to quit all fibers after they finish their current task
	// After that worker threads will stop.
	void Quit();

	// Waits for all threads to finish
	void WaitForCompletion();
private:
	void WorkerThreadEntryPoint();
	static void FiberEntryPoint(void* params);

	void CleanUpOldFiber();

	struct NextFreeFiber
	{
		FiberHandle Handle;
		unsigned Index;
	};

	struct JobData
	{
		JobDecl Job;
		Counter* Counter;
		const char* Name;
	};

	struct ReadyFiber
	{
		unsigned FiberId;
		const char* JobName;
	};

	struct WaitingFiber
	{
		unsigned FiberId;
		unsigned TargetValue;
		const char* JobName;
		std::atomic<bool> CanBeMadeReady;
	};

	NextFreeFiber GetNextFreeFiber();

	eastl::vector<std::thread> m_WorkerThreads;
	eastl::vector<FiberHandle> m_Fibers;

	Queue<JobData> m_Jobs;
	Queue<unsigned> m_FreeFibers;
	Queue<ReadyFiber> m_ReadyFibers;

	std::atomic<bool> m_Quit;

	//
	std::mutex m_WaitingFibersMutex;
	eastl::unordered_map<Counter*, WaitingFiber> m_WaitingFibers;
	//

	static const unsigned INVALID_FIBER_ID = -1;
	struct WorkerThreadData
	{
		const char* CurrentJobName = nullptr;
		FiberHandle InitialFiber = nullptr;
		unsigned CurrentFiberId = INVALID_FIBER_ID;
		unsigned FiberToPushToFreeList = INVALID_FIBER_ID;
		std::atomic<bool>* CanBeMadeReadyFlag = nullptr;
	};

	static thread_local WorkerThreadData tlsWorkerThreadData;
};
}
}