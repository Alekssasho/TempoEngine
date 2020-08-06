#pragma once

#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <filesystem>

#include <flatbuffers/flatbuffers.h>

namespace Tempest
{
class ResourceLoader
{
struct filesystem_path_hash
{
	size_t operator()(const std::filesystem::path& p) const { return std::filesystem::hash_value(p); }
};
using AssetMap = eastl::hash_map<std::filesystem::path, eastl::vector<uint8_t>, filesystem_path_hash>;

public:
	ResourceLoader(const char* dataFolder);

	template<typename ResourceType>
	const ResourceType* LoadResource(const char* fileName)
	{
		AssetMap::iterator it = EnsureResourceIsLoaded(fileName);
		if (it == m_LoadedAssets.end()) {
			return nullptr;
		}
		const ResourceType* resource = flatbuffers::GetRoot<ResourceType>(it->second.data());
#ifdef _DEBUG
		flatbuffers::Verifier verifier(it->second.data(), it->second.size());
		assert(resource->Verify(verifier));
#endif
		return resource;
	}

	AssetMap::iterator EnsureResourceIsLoaded(const char* fileName);
private:
	std::filesystem::path m_DataFolder;
	AssetMap m_LoadedAssets;
};
}