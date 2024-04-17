#include <EngineCore.h>

#include <fstream>
#include <filesystem>

int main()
{
	char exePath[MAX_PATH];
	::GetModuleFileNameA(NULL, exePath, MAX_PATH);
	std::filesystem::current_path(std::filesystem::path(exePath).parent_path());

	Tempest::EngineCoreOptions options;
	options.NumWorkerThreads = std::thread::hardware_concurrency();
	//options.NumWorkerThreads = 1;
	// Make real resource folder
	options.ResourceFolder = "../../Tempest/Shaders/";

	{
		Tempest::EngineCore engine(options);
		engine.GetJobSystem().Quit();
	}
}