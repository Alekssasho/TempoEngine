#pragma once
#include <Defines.h>
#include <Logging.h>
#include <Job/JobSystem.h>

namespace Tempest
{
struct EngineCoreOptions
{
	uint32_t numWorkerThreads;
};

class TEMPEST_API EngineCore
{
public:
	EngineCore(const EngineCoreOptions& options);
	~EngineCore();

	void StartEngineLoop();
private:
	EngineCoreOptions m_Options;

	Logger m_Logger;
	Job::JobSystem m_JobSystem;
};

static EngineCore* gEngine = nullptr;
}