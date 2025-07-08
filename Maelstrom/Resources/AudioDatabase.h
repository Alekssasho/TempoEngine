#pragma once

#include "Resource.h"
#include "Texture.h"

#include "../GLTFScene.h"

#include <Audio/AudioConfig.h>

#include <DataDefinitions/SoundDatabase_generated.h>

struct WavHeader
{
	uint8_t RIFF[4];
	uint32_t ChunkSize;
	uint8_t WAVE[4];

	uint8_t Fmt[4];
	uint32_t Subchunk1Size;
	uint16_t AudioFormat;
	uint16_t NumChannels;
	uint32_t SamplesPerSec;
	uint32_t BytesPerSec;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
};

struct WavChunkHeader
{
    uint8_t Name[4];
    uint32_t SubchunkSize;
};

struct AudioDatabaseResource : Resource<eastl::vector<uint8_t>>
{
public:
	AudioDatabaseResource()
	{}

	AudioDatabaseResource(eastl::span<eastl::string> requestedSoundClips)
		: m_RequestedSoundClips(requestedSoundClips)
	{}

	void Compile() override
	{
		// Background music
		const char* backgroundMusicFile = "file_example_oog_48.ogg";
        std::filesystem::path outputPath(gCompilerOptions->InputFolder.c_str());
        outputPath.append(backgroundMusicFile);

		uint64_t size = std::filesystem::file_size(outputPath);

		std::ifstream file(outputPath, std::ios::binary);

		// TODO: check if we can use eastl instead of std here
		eastl::vector<uint8_t> backgroundData(size);
		file.read((char*)backgroundData.data(), size);


		eastl::vector<Tempest::Definition::SoundClip> soundClips;
		eastl::vector<uint8_t> soundClipData;
		// Sound Clips
		for (const auto& soundClipName : m_RequestedSoundClips)
		{
			std::filesystem::path inputClipName(gCompilerOptions->InputFolder.c_str());
			inputClipName.append(soundClipName.c_str());

			uint64_t size = std::filesystem::file_size(inputClipName);
			std::ifstream clipFile(inputClipName, std::ios::binary);

			WavHeader header;
			clipFile.read((char*)&header, sizeof(WavHeader));

			assert(header.NumChannels == Tempest::sAudioNumChannels);
			assert(header.BitsPerSample == Tempest::sAudioClipBitDepth);
			assert(header.SamplesPerSec == Tempest::sAudioSampleRate);
			assert(header.AudioFormat == 1); // PCM

			bool hasReadData = false;
			while (!hasReadData)
			{
				WavChunkHeader chunkHeader;
				clipFile.read((char*)&chunkHeader, sizeof(WavChunkHeader));

				if (memcmp(chunkHeader.Name, "data", 4) == 0)
				{
					hasReadData = true;

					const uint32_t bytesPerSample = header.BitsPerSample / 8;
					const uint32_t numSamples = chunkHeader.SubchunkSize / bytesPerSample;
					const float duration = float(numSamples) / (header.SamplesPerSec * header.NumChannels);

					uint32_t soundClipDataStartIndex = uint32_t(soundClipData.size());
					soundClips.emplace_back(duration, soundClipDataStartIndex, chunkHeader.SubchunkSize);

					soundClipData.resize(soundClipDataStartIndex + chunkHeader.SubchunkSize);
					clipFile.read((char*)&soundClipData[soundClipDataStartIndex], chunkHeader.SubchunkSize);
				}
				else
				{
					clipFile.seekg(chunkHeader.SubchunkSize, std::ios_base::cur);
				}
			}
		}

		flatbuffers::FlatBufferBuilder builder(1024 * 1024);
        auto backgroundMusicData = builder.CreateVector<uint8_t>(backgroundData.data(), backgroundData.size());
        auto soundClipsData = builder.CreateVectorOfStructs<Tempest::Definition::SoundClip>(soundClips.data(), soundClips.size());
		auto soundClipDataArray = builder.CreateVector<uint8_t>(soundClipData.data(), soundClipData.size());
		auto root = Tempest::Definition::CreateSoundDatabase(
			builder,
			soundClipsData,
			soundClipDataArray,
			backgroundMusicData
		);
		Tempest::Definition::FinishSoundDatabaseBuffer(builder, root);

		m_CompiledData.resize(builder.GetSize());
		memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
	}

private:
	eastl::span<eastl::string> m_RequestedSoundClips;
};
