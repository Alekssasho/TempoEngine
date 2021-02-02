#pragma once
#include <cstdint>

class AudioManager
{
public:
	AudioManager();
	~AudioManager();

	void Update();
private:
	struct IAudioClient* m_AudioClient;
	struct IAudioRenderClient* m_RenderClient;
	uint32_t m_MaxFramesInBuffer = 0;
	uint32_t m_SampleRate;

	uint32_t m_SampleCount;
};

