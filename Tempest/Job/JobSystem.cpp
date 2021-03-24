#include <CommonIncludes.h>

#include <Job/JobSystem.h>

#ifdef TEMPEST_PLATFORM_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

//#define DEBUG_JOB_SYSTEM

namespace Tempest
{
namespace Job
{
// TODO: This should be in platform stuff
void SetThreadName(const char* thread)
{
	struct THREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	};

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = thread;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	const DWORD MS_VC_EXCEPTION = 0x406D1388;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

thread_local JobSystem::WorkerThreadData JobSystem::tlsWorkerThreadData;

JobSystem::~JobSystem()
{
	WaitForCompletion();

	for (auto& fiber : m_Fibers)
	{
		::DeleteFiber(fiber);
	}
}

JobSystem::JobSystem(uint32_t numWorkerThreads, uint32_t numFibers, uint32_t fiberStackSize)
	: m_Quit(false)
{
#ifdef DEBUG_JOB_SYSTEM
	return;
#endif
	// First create all the needed fibers
	m_Fibers.reserve(numFibers);

	for (auto i = 0u; i < numFibers; ++i)
	{
		m_Fibers.push_back(::CreateFiber(fiberStackSize, FiberEntryPoint, this));
		m_FreeFibers.Enqueue(i);
	}

	if (numWorkerThreads > 0)
	{
		m_WorkerThreads.reserve(numWorkerThreads);
		// Add a Windows Thread
		m_WorkerThreads.emplace_back(
			std::thread(
				&JobSystem::WorkerThreadEntryPoint,
				this,
				ThreadTag::Windows)
		);
		// Add rest of threads which are worker only
		for (auto i = 1u; i < numWorkerThreads; ++i)
		{
			m_WorkerThreads.emplace_back(
				std::thread(
					&JobSystem::WorkerThreadEntryPoint,
					this,
					ThreadTag::Worker)
			);
		}
	}
}

void JobSystem::Quit()
{
	m_Quit.store(true);
}

void JobSystem::WaitForCompletion()
{
#ifdef DEBUG_JOB_SYSTEM
	return;
#endif
	for (auto& thread : m_WorkerThreads)
	{
		thread.join();
	}

	m_WorkerThreads.clear();
}

void JobSystem::WorkerThreadEntryPoint(ThreadTag tag)
{
	OPTICK_THREAD("WorkerThread");
	SetThreadName("WorkerThread");

	tlsWorkerThreadData.Tag = tag;
	tlsWorkerThreadData.InitialFiber = ::ConvertThreadToFiber(this);

	// We are fiber now and we can schedule other fibers

	// Take a fiber and schedule it

	auto freeFiber = GetNextFreeFiber();

	tlsWorkerThreadData.CurrentFiberId = freeFiber.Index;
	::SwitchToFiber(freeFiber.Handle);

	// And we are back to clean up before the thread finishes.
	::ConvertFiberToThread();
}

JobSystem::NextFreeFiber JobSystem::GetNextFreeFiber()
{
	unsigned freeFiberIndex;
	// Busy loop until there is a free fiber
	while (!m_FreeFibers.Dequeue(freeFiberIndex));
	return NextFreeFiber{ m_Fibers[freeFiberIndex], freeFiberIndex };
}

void JobSystem::RunJobs(const char* name, JobDecl* jobs, uint32_t numJobs, Counter* counter, ThreadTag tag)
{
#ifdef DEBUG_JOB_SYSTEM
	for (auto i = 0u; i < numJobs; ++i)
	{
		jobs[i].EntryPoint(i, jobs[i].Data);
	}
	return;
#endif
	if (counter)
	{
		counter->Value.store(numJobs);
	}

	for (auto i = 0u; i < numJobs; ++i)
	{
		m_ThreadSpecificJobs[uint8_t(tag)].Jobs.Enqueue({ jobs[i], counter, name, i });
	}
}

void JobSystem::WaitForCounter(Counter* counter, uint32_t value)
{
#ifdef DEBUG_JOB_SYSTEM
	return;
#endif
	assert(counter);
	assert(tlsWorkerThreadData.CurrentJobName);
	{
		std::lock_guard<std::mutex> lock(m_WaitingFibersMutex);
		// Fast out
		if (counter->Value.load() == value)
		{
			return;
		}
		OPTICK_POP();

		assert(counter->Value.load() > value);

		// TODO: change this mutex with something better
		auto waitingFiberItr = m_WaitingFibers.insert(counter);
		waitingFiberItr->second.FiberId = tlsWorkerThreadData.CurrentFiberId;
		waitingFiberItr->second.TargetValue = value;
		waitingFiberItr->second.JobName = tlsWorkerThreadData.CurrentJobName;
		waitingFiberItr->second.CanBeMadeReady = false;

		tlsWorkerThreadData.CanBeMadeReadyFlag = &waitingFiberItr->second.CanBeMadeReady;
	}

	auto freeFiber = GetNextFreeFiber();

	tlsWorkerThreadData.CurrentFiberId = freeFiber.Index;
	::SwitchToFiber(freeFiber.Handle);

	// And we are back to clean up
	CleanUpOldFiber();
}

bool JobSystem::FiberLoopBody(JobSystem* system, ThreadQueues& jobQueues)
{
	// First check for waiting fibers
	if (!jobQueues.ReadyFibers.Empty())
	{
		ReadyFiber readyFiber;
		if (!jobQueues.ReadyFibers.Dequeue(readyFiber))
		{
			return false;
		}

		// Remember the current fiber which needs to be pushed into the free list
		// We cannot push it in the free list because another thread can take it
		// and corrupted the fiber stack before we manage to switch to another fiber
		tlsWorkerThreadData.FiberToPushToFreeList = tlsWorkerThreadData.CurrentFiberId;

		tlsWorkerThreadData.CurrentJobName = readyFiber.JobName;
		tlsWorkerThreadData.CurrentFiberId = readyFiber.FiberId;
		OPTICK_PUSH_DYNAMIC(readyFiber.JobName);

		::SwitchToFiber(system->m_Fibers[readyFiber.FiberId]);

		// And we have returned. Clean the old fiber
		system->CleanUpOldFiber();

		return true;
	}
	// Take new task
	else if (!jobQueues.Jobs.Empty())
	{
		JobData jobData;
		if (!jobQueues.Jobs.Dequeue(jobData))
		{
			return false;
		}

		tlsWorkerThreadData.CurrentJobName = jobData.Name;
		OPTICK_PUSH_DYNAMIC(jobData.Name);

		jobData.Job.EntryPoint(jobData.Index, jobData.Job.Data);

		OPTICK_POP();
		tlsWorkerThreadData.CurrentJobName = nullptr;

		// This task is done. Decrement its counter
		if (jobData.Counter)
		{
			{
				std::lock_guard<std::mutex> lock(system->m_WaitingFibersMutex);

				jobData.Counter->Value.fetch_sub(1);
				auto count = system->m_WaitingFibers.count(jobData.Counter);
				for (auto i = 0; i < count; ++i)
				{
					auto findIt = system->m_WaitingFibers.find(jobData.Counter);
					if (jobData.Counter->Value.load() <= findIt->second.TargetValue)
					{
						// Busy loop on this flag. If it is false, it means that
						// this waiting thread has not switched to another fiber
						// Adding it in the readyFibersList will expose a chance
						// to corrupt the stack of the fiber if we switch to it before
						// it has switched
						while (!findIt->second.CanBeMadeReady.load());

						jobQueues.ReadyFibers.Enqueue(ReadyFiber{ findIt->second.FiberId, findIt->second.JobName });
						system->m_WaitingFibers.erase(findIt);
					}
				}
			}
		}
		return true;
	}

	return false;
}

void JobSystem::FiberEntryPoint(void* params)
{
	auto system = reinterpret_cast<JobSystem*>(params);

	system->CleanUpOldFiber();

	while (!system->m_Quit.load())
	{
		// First try to execute from current thread specific jobs
		bool didWeRunAJob = FiberLoopBody(system, system->m_ThreadSpecificJobs[uint8_t(tlsWorkerThreadData.Tag)]);
		// If we are not Worker specific, try to execute a worker jobs as well
		if (!didWeRunAJob && tlsWorkerThreadData.Tag != ThreadTag::Worker)
		{
			FiberLoopBody(system, system->m_ThreadSpecificJobs[uint8_t(ThreadTag::Worker)]);
		}
	}

	// return to Thread fiber to finish threads
	::SwitchToFiber(tlsWorkerThreadData.InitialFiber);

	// This should not be reached
	assert(false);
}

void JobSystem::CleanUpOldFiber()
{
	if (tlsWorkerThreadData.FiberToPushToFreeList != INVALID_FIBER_ID)
	{
		m_FreeFibers.Enqueue(tlsWorkerThreadData.FiberToPushToFreeList);
		tlsWorkerThreadData.FiberToPushToFreeList = INVALID_FIBER_ID;
	}
	else if (tlsWorkerThreadData.CanBeMadeReadyFlag)
	{
		// Flag that we have switched the thread and it is safe to be put in
		// ready fibers list
		tlsWorkerThreadData.CanBeMadeReadyFlag->store(true);
		tlsWorkerThreadData.CanBeMadeReadyFlag = nullptr;
	}
}

}
}

#else
#error Implement me
#endif