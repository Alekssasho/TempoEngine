#include <EngineCore.h>

#include <filesystem>

#include "Resources/Level.h"

int main()
{
	char exePath[MAX_PATH];
	::GetModuleFileNameA(NULL, exePath, MAX_PATH);
	std::filesystem::current_path(std::filesystem::path(exePath).parent_path());

	Tempest::EngineCoreOptions options;
	options.NumWorkerThreads = std::thread::hardware_concurrency();
	// Make real resource folder
	options.ResourceFolder = "../../Tempest/Shaders/";

	{
		Tempest::EngineCore engine(options);

		eastl::string levelName("car3");
		Tempest::Job::JobDecl job{ [](uint32_t, void* levelNameData) {
			eastl::string& levelName = *(eastl::string*)levelNameData;
			LevelResource level(levelName);

			CompilerOptions options{
				.InputFolder = "../../",
				.OutputFolder = "../../Tempest/Shaders/"
			};

			gCompilerOptions = &options;

			level.Compile();
			eastl::vector<uint8_t> compiledData = level.GetCompiledData();

			std::filesystem::path outputPath(options.InputFolder.c_str());
			outputPath.append(("Level_" + levelName).c_str());
			outputPath.replace_extension(Tempest::Definition::LevelExtension());
			std::ofstream outputFile(outputPath, std::ios::binary);
			outputFile.write(reinterpret_cast<char*>(compiledData.data()), compiledData.size());

			Tempest::gEngineCore->GetJobSystem().Quit();
		}, &levelName };

		engine.GetJobSystem().RunJobs("Compile Level", &job, 1, nullptr, Tempest::Job::ThreadTag::Worker);

		engine.GetJobSystem().WaitForCompletion();
	}
}