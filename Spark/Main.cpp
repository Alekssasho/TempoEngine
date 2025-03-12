#include <Game.h>

#include <fstream>
#include <filesystem>

int main()
{
	char exePath[MAX_PATH];
	::GetModuleFileNameA(NULL, exePath, MAX_PATH);
	std::filesystem::current_path(std::filesystem::path(exePath).parent_path());

	Tempest::EngineOptions options;
	options.NumWorkerThreads = std::thread::hardware_concurrency();
	//options.NumWorkerThreads = 1;
	options.Width = 1280;
	options.Height = 720;
	// Make real resource folder
	options.ResourceFolder = "../../Tempest/Shaders/";

	Tempest::GameOptions gameOptions;
	//gameOptions.LevelToLoad = "Level_village.tlb";
    //gameOptions.LevelToLoad = "Level_car3.tlb";
    gameOptions.LevelToLoad = "Level_CastleFight.tlb";

	{
		Tempest::Game game(gameOptions, options);
		game.Start();
	}
}