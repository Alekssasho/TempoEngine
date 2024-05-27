#pragma once

#include "Resource.h"
#include "Texture.h"

#include "../GLTFScene.h"

#include <DataDefinitions/AudioDatabase_generated.h>

struct AudioDatabaseResource : Resource<eastl::vector<uint8_t>>
{
public:
	AudioDatabaseResource()
	{}

	void Compile() override
	{
		const char* backgroundMusicFile = "file_example_oog_48.ogg";

		uint64_t size = std::filesystem::file_size(backgroundMusicFile);

		std::ifstream file(backgroundMusicFile, std::ios::binary);

		// TODO: check if we can use eastl instead of std here
		std::vector<uint8_t> data(size);
		file.read((char*)data.data(), size);

		flatbuffers::FlatBufferBuilder builder(1024 * 1024);
		auto root = Tempest::Definition::CreateAudioDatabaseDirect(builder, &data);
		Tempest::Definition::FinishAudioDatabaseBuffer(builder, root);

		m_CompiledData.resize(builder.GetSize());
		memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
	}
};
