#pragma once
#include <cstdint>

struct IAudioClient;
struct IAudioRenderClient;

namespace Tempest
{
namespace Definition {
	struct AudioDatabase;
}

class AudioManager
{
public:
	AudioManager();
	~AudioManager();

	void Update();
	void LoadDatabase(const char* databaseName);
private:
	IAudioClient* m_AudioClient;
	IAudioRenderClient* m_RenderClient;
	uint32_t m_MaxFramesInBuffer = 0;
	uint32_t m_SampleRate;

	uint32_t m_SampleCount;

	const Definition::AudioDatabase* m_Database;
	// Background music
};
}

