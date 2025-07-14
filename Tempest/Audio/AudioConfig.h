#pragma once

#include <cstdint>

namespace Tempest
{
    constexpr static uint32_t sAudioSampleRate = 44100;
    constexpr static uint32_t sAudioNumChannels = 2;

    constexpr static uint32_t sAudioClipBitDepth = 16;

    // 60 Hz frame
    constexpr static uint32_t sAudioSamplesForFrame = sAudioSampleRate / 60;
}

