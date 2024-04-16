#include <CommonIncludes.h>

#include "AudioManager.h"

#include <EngineCore.h>
#include <DataDefinitions/AudioDatabase_generated.h>

//#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#include <Audioclient.h>
#include <limits>
#include <mmdeviceapi.h>


namespace Tempest
{
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

	const int numChannels = 2;
	m_SampleRate = 48000;

	WAVEFORMATEX mixFormat = {};
	mixFormat.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	mixFormat.nChannels = numChannels;
	mixFormat.nSamplesPerSec = m_SampleRate;
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
		4238,
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
}

AudioManager::~AudioManager()
{
	if(m_VorbisDecoder)
	{
		stb_vorbis_close(m_VorbisDecoder);
		m_VorbisDecoder = nullptr;
	}

	m_AudioClient->Stop();
	m_RenderClient->Release();
	m_AudioClient->Release();
}

struct AudioFrame
{
	float leftSample;
	float rightSample;
};

void AudioManager::Update()
{
	HRESULT hr = S_OK;

	uint32_t padding = 0;
	m_AudioClient->GetCurrentPadding(&padding);
	const uint32_t framesAvailable = m_MaxFramesInBuffer - padding;

	BYTE* pData = nullptr;
	if (FAILED(hr = m_RenderClient->GetBuffer(framesAvailable, &pData)))
	{
		LOG(Error, Audio, "Cannot get buffer");
		return;
	}

	auto samples = reinterpret_cast<AudioFrame*>(pData);

	uint32_t framesDecoded = stb_vorbis_get_samples_float_interleaved(m_VorbisDecoder, 2, reinterpret_cast<float*>(pData), 2 * framesAvailable);

	if(framesDecoded < framesAvailable)
	{
		// Loop the background music
		stb_vorbis_seek_start(m_VorbisDecoder);
		stb_vorbis_get_samples_float_interleaved(m_VorbisDecoder, 2, reinterpret_cast<float*>(pData) + framesDecoded * 2, 2 * (framesAvailable - framesDecoded));
	}

	//for (uint32_t i = 0; i < framesAvailable; ++i)
	//{
	//	//float sineWave = sinf(m_SampleCount * 2.0f * glm::pi<float>() * 110.0f / float(m_SampleRate));
	//	//sineWave *= 0.1f;
	//	//samples[i].leftSample = sineWave;
	//	//samples[i].rightSample = sineWave;
	//	//++m_SampleCount;
	//}

	if (FAILED(hr = m_RenderClient->ReleaseBuffer(framesAvailable, 0)))
	{
		LOG(Error, Audio, "Cannot release buffer");
		return;
	}
}

void AudioManager::LoadDatabase(const char* databaseName)
{
	const Definition::AudioDatabase* audioDatabase = gEngine->GetResourceLoader().LoadResource<Definition::AudioDatabase>(databaseName);
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
}
