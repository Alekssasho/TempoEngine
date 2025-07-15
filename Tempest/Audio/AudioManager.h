#pragma once
#include <cstdint>
#include <Audio/AudioConfig.h>

struct IAudioClient;
struct IAudioRenderClient;
struct stb_vorbis;

namespace Tempest
{
namespace Definition {
	struct SoundDatabase;
}

struct PlayingSoundEffect
{
	uint32_t SoundEffectIndex;
	uint32_t CurrentSample;
	float VolumeLeft;
	float VolumeRight;
};

struct AudioFrame
{
	float leftSample;
	float rightSample;
};

struct AudioSamplesRingBuffer
{
	AudioFrame* Samples;

	std::atomic<uint32_t> ReadIndex;
	std::atomic<uint32_t> WriteIndex;
	uint32_t Size;
};

class AudioManager
{
public:
	AudioManager();
	~AudioManager();

	void Update();
	void LoadDatabase(const char* databaseName);
	void PlaySoundEffect(uint32_t soundEffectIndex, float volumeLeft, float volumeRight);
	eastl::vector<PlayingSoundEffect> m_CurrentPlayingSounds;
private:
	void PrepareNextFrameAudio();
	void WriteToAudioBuffer();


	IAudioClient* m_AudioClient;
	IAudioRenderClient* m_RenderClient;
	uint32_t m_MaxFramesInBuffer = 0;

	AudioSamplesRingBuffer m_RingBuffer;

	//uint32_t m_SampleCount;

	const Definition::SoundDatabase* m_Database;
	// Background music
	stb_vorbis* m_VorbisDecoder;

};
}

