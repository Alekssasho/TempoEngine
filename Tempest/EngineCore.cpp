#include <CommonIncludes.h>

#include <EngineCore.h>
#include <Memory.h>
#include <optick.h>

namespace Tempest
{
EngineCore* gEngineCore = nullptr;

EngineCore::EngineCore(const EngineCoreOptions& options)
	: m_CoreOptions(options)
	, m_Logger()
	, m_JobSystem(options.NumWorkerThreads, 64, 2 * 1024 * 1024)
	, m_ResourceLoader(options.ResourceFolder)
{
	gEngineCore = this;
}

EngineCore::~EngineCore()
{
}
}