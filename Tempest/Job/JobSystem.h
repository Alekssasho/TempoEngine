#pragma once

#include <atomic>
#include <thread>
#include <mutex>

#include <Job/Queue.h>

namespace Tempest
{
namespace Job
{

using JobEntryPoint = void(*)(uint32_t, void*);

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

// TODO: This should be client provided if this is made into a library
enum class ThreadTag : uint8_t
{
	Worker, // Standard Worker thread
	Windows, // Thread that executes windows calls as the message pump is thread specific
	Count
};

using FiberHandle = void*;

class TEMPEST_API JobSystem
{
public:
	JobSystem(uint32_t numWorkerThreads, uint32_t numFibers, uint32_t fiberStackSize);
	~JobSystem();

	// Can be called from anywhere
	void RunJobs(const char* name, JobDecl* jobs, uint32_t numJobs, Counter* counter = nullptr, ThreadTag threadToRunOn = ThreadTag::Worker);

	// Note only 1 job can wait on some counter. This sounds reasonable for now. Implement if ever needed.
	// Can be called only from a Job
	void WaitForCounter(Counter* counter, uint32_t value);

	// Will set internal flag to quit all fibers after they finish their current task
	// After that worker threads will stop.
	void Quit();

	// Waits for all threads to finish
	void WaitForCompletion();
private:
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
		uint32_t Index;
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

	struct ThreadQueues
	{
		Queue<JobData> Jobs;
		Queue<ReadyFiber> ReadyFibers;
	};

	void WorkerThreadEntryPoint(ThreadTag tag);
	static void FiberEntryPoint(void* params);
	// Returns whether we have executed a fiber
	static bool FiberLoopBody(JobSystem* system, ThreadQueues& jobQueues);

	void CleanUpOldFiber();
	NextFreeFiber GetNextFreeFiber();

	eastl::vector<std::thread> m_WorkerThreads;
	eastl::vector<FiberHandle> m_Fibers;

	Queue<unsigned> m_FreeFibers;

	eastl::array<ThreadQueues, uint8_t(ThreadTag::Count)> m_ThreadSpecificJobs;

	std::atomic<bool> m_Quit;

	//
	std::mutex m_WaitingFibersMutex;
	eastl::unordered_multimap<Counter*, WaitingFiber> m_WaitingFibers;
	//

	static const unsigned INVALID_FIBER_ID = -1;
	struct WorkerThreadData
	{
		const char* CurrentJobName = nullptr;
		FiberHandle InitialFiber = nullptr;
		unsigned CurrentFiberId = INVALID_FIBER_ID;
		unsigned FiberToPushToFreeList = INVALID_FIBER_ID;
		std::atomic<bool>* CanBeMadeReadyFlag = nullptr;
		ThreadTag Tag;
	};

	static thread_local WorkerThreadData tlsWorkerThreadData;

public:
	// Convenience functions
	template<typename Func, typename Arg>
	void WaitSingleJob(const char* jobName, ThreadTag tag, Arg& arg, Func func)
	{
		struct PassedData
		{
			Func Function;
			Arg& Argument;
		} data{ func, arg };

		Job::Counter counter;
		Job::JobDecl job{ [](uint32_t, void* data) {
			PassedData* passedData = reinterpret_cast<PassedData*>(data);
			passedData->Function(passedData->Argument);
		}, &data };
		RunJobs(jobName, &job, 1, &counter, tag);
		WaitForCounter(&counter, 0);
	}
};


}
}