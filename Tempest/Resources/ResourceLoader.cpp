#include <Resources/ResourceLoader.h>
#include <Logging.h>
#include <fstream>

namespace Tempest
{
ResourceLoader::ResourceLoader(const char* dataFolder)
	: m_DataFolder(dataFolder)
{
	if (!std::filesystem::is_directory(m_DataFolder)) {
		FORMAT_LOG(Fatal, Resources, "Given asset folder (%s) is not a directory! Aborting.", m_DataFolder.c_str());
		std::exit(1);
	}
	// TODO: Go through the directory and index the files
}

ResourceLoader::AssetMap::iterator ResourceLoader::EnsureResourceIsLoaded(const char* fileName)
{
	std::filesystem::path filePath = m_DataFolder / fileName;

	AssetMap::iterator findItr = m_LoadedAssets.find(filePath);
	// We have the file already loaded
	if (findItr != m_LoadedAssets.end()) {
		return findItr;
	}

	if (!std::filesystem::is_regular_file(filePath)) {
		FORMAT_LOG(Error, Resources, "Given filepath (%s) is not a file.", filePath.c_str());
		return m_LoadedAssets.end(); // invalid end iterator
	}

	uint64_t size = std::filesystem::file_size(filePath);

	std::ifstream stream(filePath, std::ios::binary);
	assert(stream.good());

	eastl::vector<uint8_t> data(size);
	stream.read((char*)data.data(), size);

	auto[it, didInsert] = m_LoadedAssets.emplace(filePath, std::move(data));
	assert(didInsert);
	return it;
}
}