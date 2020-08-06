#include <EngineCore.h>

int main()
{
	Tempest::EngineCoreOptions options;
	options.NumWorkerThreads = std::thread::hardware_concurrency();
	//options.NumWorkerThreads = 1;
	options.Width = 1280;
	options.Height = 720;
	// TODO: Only resource for now are shaders so just point to that folder.
	// Make real resource folder
	options.ResourceFolder = "../../Tempest/Shaders/";

	{
		Tempest::EngineCore engine(options);
		engine.StartEngineLoop();
	}
}