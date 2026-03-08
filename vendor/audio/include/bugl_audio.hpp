#pragma once

#include <string>

namespace bugl::audio
{
    class Engine
    {
    public:
        struct Impl;
        using SoundId = int;
        using Handle = int;

        static constexpr int WAVE_SINE = 0;
        static constexpr int WAVE_SQUARE = 1;
        static constexpr int WAVE_TRIANGLE = 2;
        static constexpr int WAVE_SAW = 3;

        static constexpr int NOISE_WHITE = 0;
        static constexpr int NOISE_PINK = 1;
        static constexpr int NOISE_BROWNIAN = 2;

        Engine();
        ~Engine();

        Engine(const Engine &) = delete;
        Engine &operator=(const Engine &) = delete;
        Engine(Engine &&) = delete;
        Engine &operator=(Engine &&) = delete;

        bool init();
        void shutdown();
        bool isReady() const;

        SoundId createSfx(const std::string &path);
        SoundId createMusic(const std::string &path);
        SoundId createWaveform(int type, float amplitude, float frequency);
        SoundId createNoise(int type, int seed, float amplitude);
        bool removeSound(SoundId soundId);
        void clearSoundBank();

        Handle playSfx(SoundId soundId, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f);
        Handle playMusic(SoundId soundId, bool loop = true, float volume = 1.0f);
        void stopMusic();
        bool isMusicPlaying() const;

        bool stop(Handle handle);
        bool pause(Handle handle);
        bool resume(Handle handle);
        bool setVolume(Handle handle, float volume);
        bool setPitch(Handle handle, float pitch);
        bool setPan(Handle handle, float pan);
        bool isPlaying(Handle handle) const;

        void setMasterVolume(float volume);
        void setSfxVolume(float volume);
        void setMusicVolume(float volume);
        void enableSfxDelay(bool enable, float decay = 0.5f);
        void enableMusicLowPass(bool enable, float cutoff = 1000.0f);

        void stopAll();
        void update();

    private:
        Impl *impl;
    };
}
