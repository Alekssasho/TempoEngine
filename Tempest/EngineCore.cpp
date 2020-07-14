#include <EngineCore.h>
#include <Memory.h>
#include <optick.h>

namespace Tempest
{
EngineCore::EngineCore(const EngineCoreOptions& options)
	: m_Options(options)
	, m_Logger()
	, m_JobSystem(options.numWorkerThreads, 64, 2 * 1024 * 1024)
{
	gEngine = this;
}

EngineCore::~EngineCore()
{
}

void EngineCore::StartEngineLoop()
{

	m_JobSystem.WaitForCompletion();
}
}