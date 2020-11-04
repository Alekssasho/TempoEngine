#include <EngineCore.h>

#include <fstream>

int main()
{
	Tempest::EngineCoreOptions options;
	options.NumWorkerThreads = std::thread::hardware_concurrency();
	//options.NumWorkerThreads = 1;
	options.Width = 1280;
	options.Height = 720;
	// Make real resource folder
	options.ResourceFolder = "../../Tempest/Shaders/";
	options.LevelToLoad = "Level_Duck.tlb";

	{
		Tempest::EngineCore engine(options);
		engine.StartEngineLoop();
	}
}