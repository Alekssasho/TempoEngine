#include <CommonIncludes.h>

#include "AudioManager.h"

#include <Engine.h>
#include <DataDefinitions/SoundDatabase_generated.h>

#include <imgui.h>

//#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#include <Audioclient.h>
#include <limits>
#include <mmdeviceapi.h>

#include <immintrin.h>

namespace Tempest
{
struct CombFilter
{
	eastl::vector<float> DelayLine;
	uint32_t CurrentIndex;
	float Feedback;
	float Damping;
	float LPFState;

	void Initialize(uint32_t delaySize, float feedback, float damping)
	{
		DelayLine.resize(delaySize, 0.0f);
		CurrentIndex = 0;
		Feedback = feedback;
		Damping = damping;
		LPFState = 0.0f;
	}

	float Process(float input)
	{
		float delayed = DelayLine[CurrentIndex];

		DelayLine[CurrentIndex] = input + (Feedback * LPFState);
		LPFState = delayed * (1.0f - Damping) + LPFState * Damping;

		CurrentIndex = (CurrentIndex + 1) % DelayLine.size();
		return delayed;
	}
};

struct AllPassFilter
{
	eastl::vector<float> DelayLine;
	uint32_t CurrentIndex;
	float Gain;

	void Initialize(uint32_t delaySize, float gain)
	{
		DelayLine.resize(delaySize, 0.0f);
		CurrentIndex = 0;
		Gain = gain;
	}

	float Process(float input)
	{
		float delayed = DelayLine[CurrentIndex];
		float output = input + delayed * Gain;
		DelayLine[CurrentIndex] = output;

		CurrentIndex = (CurrentIndex + 1) % DelayLine.size();
		return output * -Gain + delayed;
	}
};

struct Reverb
{
	eastl::array<CombFilter, 4> combL;
	eastl::array<CombFilter, 4> combR;
	eastl::array<AllPassFilter, 2> apfL;
	eastl::array<AllPassFilter, 2> apfR;

	float Dry;
	float Wet;

	Reverb()
	{
		Dry = 0.7f;
		Wet = 0.3f;

		float combFeedback = 0.75f;
		float combDamping = 0.3f;
		combL[0].Initialize(1557, combFeedback, combDamping);
		combL[1].Initialize(1617, combFeedback, combDamping);
		combL[2].Initialize(1491, combFeedback, combDamping);
		combL[3].Initialize(1422, combFeedback, combDamping);

        combR[0].Initialize(1557, combFeedback, combDamping);
        combR[1].Initialize(1617, combFeedback, combDamping);
        combR[2].Initialize(1491, combFeedback, combDamping);
        combR[3].Initialize(1422, combFeedback, combDamping);

		float apfGain = 0.6f;
        apfL[0].Initialize(225, apfGain);
        apfL[1].Initialize(556, apfGain);

        apfR[0].Initialize(225, apfGain);
        apfR[1].Initialize(556, apfGain);
	}

	AudioFrame Process(AudioFrame input)
	{
		AudioFrame output = { 0.0f, 0.0f };

		for (uint32_t i = 0; i < 4; ++i)
		{
			output.leftSample += combL[i].Process(input.leftSample);
			output.rightSample += combR[i].Process(input.rightSample);
		}

		for (uint32_t i = 0; i < 2; ++i)
		{
			output.leftSample = apfL[i].Process(output.leftSample);
			output.rightSample = apfR[i].Process(output.rightSample);
		}

		return AudioFrame{
			input.leftSample * Dry + output.leftSample * Wet,
			input.rightSample * Dry + output.rightSample * Wet,
		};
	}
};

AudioManager::AudioManager()
	: m_VorbisDecoder(nullptr)
{
	HRESULT hr = S_OK;

	// Initialize Windows COM
	// TODO: This should be in Windows specific file
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// Getting enumerator
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	IMMDeviceEnumerator* pEnumerator = nullptr;
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	if (FAILED(hr))
	{
		LOG(Error, Audio, "Cannot create device enumerator");
		return;
	}

	IMMDevice* defaultDevice = nullptr;
	if (FAILED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice)))
	{
		LOG(Error, Audio, "Cannot get default device endpoint");
		return;
	}

	if (FAILED(defaultDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_AudioClient)))
	{
		LOG(Error, Audio, "Cannot activate audio client");
		return;
	}

	WAVEFORMATEX mixFormat = {};
	mixFormat.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	mixFormat.nChannels = sAudioNumChannels;
	mixFormat.nSamplesPerSec = sAudioSampleRate;
	mixFormat.wBitsPerSample = 32;
	mixFormat.nBlockAlign = (mixFormat.nChannels * mixFormat.wBitsPerSample) / 8;
	mixFormat.nAvgBytesPerSec = mixFormat.nSamplesPerSec * mixFormat.nBlockAlign;
	mixFormat.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMAT);

	WAVEFORMATEXTENSIBLE extFormat = {};
	extFormat.Format = mixFormat;
	extFormat.Samples.wValidBitsPerSample = mixFormat.wBitsPerSample;
	extFormat.dwChannelMask = KSAUDIO_SPEAKER_DIRECTOUT;
	extFormat.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

	if (FAILED(hr = m_AudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
		0,
		0,
		(const WAVEFORMATEX*)&extFormat,
		nullptr)))
	{
		LOG(Error, Audio, "Cannot initialize audio client");
		return;
	}

	if (FAILED(m_AudioClient->GetBufferSize(&m_MaxFramesInBuffer)))
	{
		LOG(Error, Audio, "Cannot get buffer size");
		return;
	}

	if (FAILED(m_AudioClient->GetService(IID_PPV_ARGS(&m_RenderClient))))
	{
		LOG(Error, Audio, "Cannot get render client");
		return;
	}

	m_AudioClient->Start();

	defaultDevice->Release();
	pEnumerator->Release();

	m_RingBuffer.ReadIndex.store(0);
	m_RingBuffer.WriteIndex.store(sAudioSamplesForFrame * 3); // Start with 10 frames data for start
	m_RingBuffer.Samples = reinterpret_cast<AudioFrame*>(_aligned_malloc(sizeof(AudioFrame) * sAudioSampleRate, 32)); // 1 second of data
	m_RingBuffer.Size = sAudioSampleRate;

	m_Reverb.reset(new Reverb);
}

AudioManager::~AudioManager()
{
	_aligned_free(m_RingBuffer.Samples);
	m_RingBuffer.Samples = nullptr;

	if(m_VorbisDecoder)
	{
		stb_vorbis_close(m_VorbisDecoder);
		m_VorbisDecoder = nullptr;
	}

	m_AudioClient->Stop();
	m_RenderClient->Release();
	m_AudioClient->Release();
}

void AudioManager::PlaySoundEffect(uint32_t soundEffectIndex, float volumeLeft, float volumeRight)
{
	m_CurrentPlayingSounds.emplace_back(soundEffectIndex, 0, volumeLeft, volumeRight);
}


float ConvertPCM16ToFloat(const int16_t* input) {
    return static_cast<float>(*input) / 32768.0f;
}

void AudioManager::PrepareNextFrameAudio()
{
	ZoneScoped;
	// Write only data for 0.33ms
	constexpr uint32_t desiredSamplesToWrite = sAudioSamplesForFrame * 2;
	uint32_t writeIndex = m_RingBuffer.WriteIndex.load();
	static_assert(desiredSamplesToWrite % 8 == 0);

	bool writeInBeginning = false;
	uint32_t samplesToWrite = desiredSamplesToWrite;
	if (writeIndex + samplesToWrite > m_RingBuffer.Size)
	{
		samplesToWrite = m_RingBuffer.Size - writeIndex;
		writeInBeginning = true;
	}
	AudioFrame* samples = m_RingBuffer.Samples + writeIndex;

	assert(samplesToWrite % 8 == 0);

    for (uint32_t i = 0; i < samplesToWrite; i += 4)
    {
		__m256 zero = _mm256_setzero_ps();
		_mm256_store_ps(reinterpret_cast<float*>(samples + i), zero);
        //samples[i].leftSample = 0.0f;
        //samples[i].rightSample = 0.0f;
    }

    eastl::vector<PlayingSoundEffect> currentEffects;
    currentEffects.swap(m_CurrentPlayingSounds);

    for (auto& playingEffect : currentEffects)
    {
        const auto& effect = (*m_Database->sound_clips())[playingEffect.SoundEffectIndex];
        const auto& totalFrames = (effect->count() / (sAudioClipBitDepth / 8)) / sAudioNumChannels;
        const auto& effectFramesAvailable = totalFrames - playingEffect.CurrentSample;
        const auto& effectSamplesHalf = reinterpret_cast<const uint32_t*>(m_Database->sound_clip_data()->data() + effect->start_index());

		uint32_t samplesLeftToWrite = std::min(samplesToWrite, effectFramesAvailable);
		uint32_t leftovers = samplesLeftToWrite % 8;
        for (uint32_t i = 0; i < (samplesLeftToWrite - leftovers); i += 8)
        {
			__m256i effectSamples = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(effectSamplesHalf + playingEffect.CurrentSample + i));
			__m256 loSamples = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(effectSamples, 0)));
			__m256 hiSamples = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(effectSamples, 1)));
			__m256 normalization = _mm256_set1_ps(1.0f / 32768.0f);
			loSamples = _mm256_mul_ps(loSamples, normalization);
			hiSamples = _mm256_mul_ps(hiSamples, normalization);

			__m256 volumeInterleaved = _mm256_blend_ps(_mm256_set1_ps(playingEffect.VolumeLeft), _mm256_set1_ps(playingEffect.VolumeRight), 0b10101010);

			__m256 outputLoSamples = _mm256_load_ps(reinterpret_cast<float*>(samples + i));
			__m256 outputHiSamples = _mm256_load_ps(reinterpret_cast<float*>(samples + i + 4));

            _mm256_store_ps(reinterpret_cast<float*>(samples + i), _mm256_fmadd_ps(loSamples, volumeInterleaved, outputLoSamples));
            _mm256_store_ps(reinterpret_cast<float*>(samples + i + 4), _mm256_fmadd_ps(hiSamples, volumeInterleaved, outputHiSamples));

            //const auto& currentSample = reinterpret_cast<const int16_t*>(effectSamplesHalf + playingEffect.CurrentSample + i);
            //samples[i].leftSample += ConvertPCM16ToFloat(currentSample) * playingEffect.VolumeLeft;
            //samples[i].rightSample += ConvertPCM16ToFloat(currentSample + 1) * playingEffect.VolumeRight;
        }

		for (uint32_t i = samplesLeftToWrite - leftovers; i < samplesLeftToWrite; ++i)
		{
            const auto& currentSample = reinterpret_cast<const int16_t*>(effectSamplesHalf + playingEffect.CurrentSample + i);
            samples[i].leftSample += ConvertPCM16ToFloat(currentSample) * playingEffect.VolumeLeft;
            samples[i].rightSample += ConvertPCM16ToFloat(currentSample + 1) * playingEffect.VolumeRight;
		}

        playingEffect.CurrentSample += samplesToWrite;
        if (playingEffect.CurrentSample < totalFrames)
        {
            m_CurrentPlayingSounds.emplace_back(playingEffect);
        }
    }

	if (gEngine->GetDebug().AudioUseReverb)
	{
		for (uint32_t i = 0; i < samplesToWrite; ++i)
		{
			samples[i] = m_Reverb->Process(samples[i]);
		}
	}

    //uint32_t framesDecoded = stb_vorbis_get_samples_float_interleaved(m_VorbisDecoder, 2, reinterpret_cast<float*>(pData), 2 * framesAvailable);

    //if(framesDecoded < framesAvailable)
    //{
    //	// Loop the background music
    //	stb_vorbis_seek_start(m_VorbisDecoder);
    //	stb_vorbis_get_samples_float_interleaved(m_VorbisDecoder, 2, reinterpret_cast<float*>(pData) + framesDecoded * 2, 2 * (framesAvailable - framesDecoded));
    //}

    //for (uint32_t i = 0; i < framesAvailable; ++i)
    //{
    //	//float sineWave = sinf(m_SampleCount * 2.0f * glm::pi<float>() * 110.0f / float(m_SampleRate));
    //	//sineWave *= 0.1f;
    //	//samples[i].leftSample = sineWave;
    //	//samples[i].rightSample = sineWave;
    //	//++m_SampleCount;
    //}

	// TODO: Make writing to the beginning as well

    if (writeIndex + samplesToWrite >= m_RingBuffer.Size)
    {
        m_RingBuffer.WriteIndex.store(0);
    }
    else
    {
        m_RingBuffer.WriteIndex.fetch_add(samplesToWrite);
    }
}

void AudioManager::WriteToAudioBuffer()
{
	ZoneScoped;
	HRESULT hr = S_OK;

	uint32_t padding = 0;
	m_AudioClient->GetCurrentPadding(&padding);
	uint32_t framesAvailable = m_MaxFramesInBuffer - padding;

	BYTE* pData = nullptr;
	if (FAILED(hr = m_RenderClient->GetBuffer(framesAvailable, &pData)))
	{
		LOG(Error, Audio, "Cannot get buffer");
		return;
	}

	FORMAT_LOG(Trace, Audio, "Write to Audio Buffer with %d available", framesAvailable);

	auto samples = reinterpret_cast<AudioFrame*>(pData);

	auto readIndex = m_RingBuffer.ReadIndex.load();
	auto writeIndex = m_RingBuffer.WriteIndex.load();

	bool readFromBegining = false;
	uint32_t availableSamples = 0;
	if (writeIndex > readIndex)
	{
		availableSamples = writeIndex - readIndex;
	}
	else
	{
		availableSamples = m_RingBuffer.Size - readIndex;
		readFromBegining = true;
	}

	auto totalNumberOfSamplesWritten = 0;
	auto samplesToCopy = std::min(availableSamples, framesAvailable);
	memcpy(samples, m_RingBuffer.Samples + readIndex, samplesToCopy * sizeof(AudioFrame));
	framesAvailable -= samplesToCopy;
	samples += samplesToCopy;
	m_RingBuffer.ReadIndex.fetch_add(samplesToCopy);
	totalNumberOfSamplesWritten += samplesToCopy;

	if (framesAvailable > 0 && readFromBegining)
	{
		readIndex = 0;
		availableSamples = writeIndex - readIndex;

		samplesToCopy = std::min(availableSamples, framesAvailable);
		memcpy(samples, m_RingBuffer.Samples + readIndex, samplesToCopy * sizeof(AudioFrame));

		m_RingBuffer.ReadIndex.store(samplesToCopy);

		totalNumberOfSamplesWritten += samplesToCopy;
	}
	else if(framesAvailable > 0)
	{
		FORMAT_LOG(Warning, Audio, "Audio Starvation: %d left", framesAvailable);
		for (uint32_t i = 0; i < framesAvailable; ++i)
		{
			samples[i].leftSample = 0.0f;
			samples[i].rightSample = 0.0f;
		}
	}

	if (FAILED(hr = m_RenderClient->ReleaseBuffer(totalNumberOfSamplesWritten, 0)))
	{
		LOG(Error, Audio, "Cannot release buffer");
		return;
	}
}

void AudioManager::Update()
{
	ZoneScoped;
    auto readIndex = m_RingBuffer.ReadIndex.load();
    auto writeIndex = m_RingBuffer.WriteIndex.load();
	bool shouldPrepareNewData = false;
	if (writeIndex >= readIndex)
	{
		shouldPrepareNewData = (writeIndex - readIndex) < (sAudioSamplesForFrame * 10);
	}
	else
	{
		uint32_t availableSamples = m_RingBuffer.Size - readIndex + writeIndex;
		shouldPrepareNewData = availableSamples < (sAudioSamplesForFrame * 10);
	}

	FORMAT_LOG(Trace, Audio, "Audio Update : %d read, %d write, %d will update", readIndex, writeIndex, shouldPrepareNewData);

	if (shouldPrepareNewData)
	{
		PrepareNextFrameAudio();
	}

	WriteToAudioBuffer();
}

void AudioManager::LoadDatabase(const char* databaseName)
{
	const Definition::SoundDatabase* audioDatabase = gEngineCore->GetResourceLoader().LoadResource<Definition::SoundDatabase>(databaseName);
	if (!audioDatabase)
	{
		LOG(Warning, Renderer, "Audio Database is Invalid!");
		return;
	}

	m_Database = audioDatabase;

	// TODO: Add temp memory or some kind of managed memory
	int vorbisError = 0;
	m_VorbisDecoder = stb_vorbis_open_memory(m_Database->background_music()->data(), m_Database->background_music()->size(), &vorbisError, nullptr);
	if(m_VorbisDecoder == nullptr)
	{
		LOG(Error, Audio, "Cannot vorbis decode background music");
		return;
	}
	stb_vorbis_info backgroundMusicInfo = stb_vorbis_get_info(m_VorbisDecoder);

	// We can only play 48kHz files. Thunder should have re-sampled it before adding to the database.
	assert(backgroundMusicInfo.sample_rate == 48000 && backgroundMusicInfo.channels == 2);
	FORMAT_LOG(Info, Audio, "Background music started decoding with %d channels and %d sample rate", backgroundMusicInfo.channels, backgroundMusicInfo.sample_rate);
}

void AudioManager::DebugDisplay()
{
	ImGui::SliderFloat("Reverb Dry", &m_Reverb->Dry, 0.0f, 1.0f);
	ImGui::SliderFloat("Reverb Wet", &m_Reverb->Wet, 0.0f, 1.0f);
}
}
