#include <EngineCore.h>

int main()
{
	Tempest::EngineCoreOptions options;
	options.numWorkerThreads = std::thread::hardware_concurrency();

	Tempest::EngineCore engine(options);
	engine.StartEngineLoop();
}