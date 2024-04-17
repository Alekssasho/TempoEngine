#pragma once
#include <CommonIncludes.h>

#include <Job/JobSystem.h>
#include <Platform/WindowsPlatform.h>
#include <Resources/ResourceLoader.h>
#include <InputManager.h>

namespace Tempest
{
struct EngineCoreOptions
{
	uint32_t NumWorkerThreads;
	const char* ResourceFolder;
};

class TEMPEST_API EngineCore
{
public:
	EngineCore(const EngineCoreOptions& options);
	~EngineCore();

	ResourceLoader& GetResourceLoader()
	{
		return m_ResourceLoader;
	}

	EngineCoreOptions& GetOptions()
	{
		return m_CoreOptions;
	}

	Job::JobSystem& GetJobSystem()
	{
		return m_JobSystem;
	}
protected:
	// Data members
	EngineCoreOptions m_CoreOptions;

	Logger m_Logger;
	Job::JobSystem m_JobSystem;
	ResourceLoader m_ResourceLoader;
};

extern EngineCore* gEngineCore;
}
