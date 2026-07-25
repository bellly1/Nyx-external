#pragma once

#include <Windows.h>
#include <mmsystem.h>
#include <cmath>
#include <thread>
#include <atomic>

#pragma comment(lib, "winmm.lib")

namespace HitSounds
{
    inline std::atomic<int> s_playing{ 0 };

    inline void Play(int index, float volume)
    {
        if (volume <= 0.01f) return;
        if (s_playing.load(std::memory_order_relaxed) > 0) return;
        s_playing.store(1, std::memory_order_relaxed);

        std::thread([](int idx, float vol) {
            constexpr int kRate = 44100;
            constexpr int kBits = 16;
            constexpr int kChannels = 1;
            constexpr int kBlockAlign = kChannels * (kBits / 8);

            int durationMs = 0;
            float freq1 = 0.f, freq2 = 0.f;
            float decay = 0.f;
            float amp = 1.f;

            switch (idx)
            {
            case 0: durationMs = 45; freq1 = 1800.f; freq2 = 3600.f; decay = 12.f; amp = 0.9f; break;
            case 1: durationMs = 35; freq1 = 2200.f; freq2 = 4400.f; decay = 16.f; amp = 0.85f; break;
            case 2: durationMs = 50; freq1 = 1400.f; freq2 = 2800.f; decay = 10.f; amp = 0.8f; break;
            case 3: durationMs = 30; freq1 = 2600.f; freq2 = 5200.f; decay = 20.f; amp = 0.95f; break;
            default: durationMs = 40; freq1 = 1800.f; freq2 = 3600.f; decay = 12.f; amp = 0.85f; break;
            }

            const int numSamples = (kRate * durationMs) / 1000;
            const int dataSize = numSamples * kBlockAlign;
            const int fileSize = 36 + dataSize;

            unsigned char header[44]{};

            header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
            header[4] = (unsigned char)(fileSize);
            header[5] = (unsigned char)(fileSize >> 8);
            header[6] = (unsigned char)(fileSize >> 16);
            header[7] = (unsigned char)(fileSize >> 24);
            header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';

            header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
            header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
            header[20] = 1; header[21] = 0;
            header[22] = (unsigned char)kChannels; header[23] = 0;
            const int byteRate = kRate * kBlockAlign;
            header[24] = (unsigned char)(kRate);
            header[25] = (unsigned char)(kRate >> 8);
            header[26] = (unsigned char)(kRate >> 16);
            header[27] = (unsigned char)(kRate >> 24);
            header[28] = (unsigned char)(byteRate);
            header[29] = (unsigned char)(byteRate >> 8);
            header[30] = (unsigned char)(byteRate >> 16);
            header[31] = (unsigned char)(byteRate >> 24);
            header[32] = (unsigned char)kBlockAlign; header[33] = 0;
            header[34] = (unsigned char)kBits; header[35] = 0;

            header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
            header[40] = (unsigned char)(dataSize);
            header[41] = (unsigned char)(dataSize >> 8);
            header[42] = (unsigned char)(dataSize >> 16);
            header[43] = (unsigned char)(dataSize >> 24);

            const int totalSize = 44 + dataSize;
            unsigned char* buf = new unsigned char[totalSize];
            std::memcpy(buf, header, 44);

            const float invRate = 1.f / (float)kRate;
            const float scaledAmp = amp * vol * 32000.f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float t = (float)i * invRate;
                const float env = std::exp(-decay * t);
                const float s = env * (
                    0.6f * std::sin(2.f * 3.14159265f * freq1 * t) +
                    0.4f * std::sin(2.f * 3.14159265f * freq2 * t)
                );
                float val = s * scaledAmp;
                if (val > 32767.f) val = 32767.f;
                if (val < -32768.f) val = -32768.f;
                short sample = (short)val;
                buf[44 + i * 2] = (unsigned char)(sample & 0xFF);
                buf[44 + i * 2 + 1] = (unsigned char)((sample >> 8) & 0xFF);
            }

            PlaySoundA(reinterpret_cast<LPCSTR>(buf), nullptr,
                SND_MEMORY | SND_ASYNC | SND_NOSTOP);

            Sleep(durationMs + 50);
            delete[] buf;
            s_playing.store(0, std::memory_order_relaxed);
        }, index, volume).detach();
    }
}
