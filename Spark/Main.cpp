#include <EngineCore.h>

int main()
{
	Tempest::EngineCoreOptions options;
	options.NumWorkerThreads = std::thread::hardware_concurrency();
	//options.NumWorkerThreads = 1;
	options.Width = 1280;
	options.Height = 720;

	{
		Tempest::EngineCore engine(options);
		engine.StartEngineLoop();
	}
}