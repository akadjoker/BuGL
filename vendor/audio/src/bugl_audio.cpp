#include "bugl_audio.hpp"

#include "stb_vorbis.c"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace bugl::audio
{
    namespace
    {
        constexpr float kMinPitch = 0.01f;

        float clampRange(float value, float minValue, float maxValue)
        {
            if (value < minValue)
                return minValue;
            if (value > maxValue)
                return maxValue;
            return value;
        }
    }

    // Wrapper for procedural audio sources
    struct ProceduralSource
    {
        virtual ~ProceduralSource() = default;
        virtual ma_data_source *getDataSource() = 0;
    };

    struct WaveformSource : ProceduralSource
    {
        ma_waveform waveform;
        WaveformSource(ma_format format, ma_uint32 channels, ma_uint32 sampleRate, ma_waveform_type type, double amplitude, double frequency)
        {
            ma_waveform_config config = ma_waveform_config_init(format, channels, sampleRate, type, amplitude, frequency);
            ma_waveform_init(&config, &waveform);
        }
        ~WaveformSource() { ma_waveform_uninit(&waveform); }
        ma_data_source *getDataSource() override { return &waveform.ds; }
    };

    struct NoiseSource : ProceduralSource
    {
        ma_noise noise;
        NoiseSource(ma_format format, ma_uint32 channels, ma_noise_type type, ma_int32 seed, double amplitude)
        {
            ma_noise_config config = ma_noise_config_init(format, channels, type, seed, amplitude);
            ma_noise_init(&config, nullptr, &noise);
        }
        ~NoiseSource() { ma_noise_uninit(&noise, nullptr); }
        ma_data_source *getDataSource() override { return &noise.ds; }
    };

    struct Engine::Impl
    {
        struct SoundDef
        {
            SoundId id = 0;
            bool isMusic = false;
            std::string path;
            ProceduralSource *procedural = nullptr;

            ~SoundDef() { delete procedural; }
        };

        struct Voice
        {
            Handle handle = 0;
            SoundId soundId = 0;
            ma_sound sound{};
            bool looping = false;
            bool isMusic = false;
        };

        ma_engine engine{};
        ma_sound_group sfxGroup{};
        ma_sound_group musicGroup{};

        // Effects
        ma_delay_node delayNode{};
        bool delayEnabled = false;
        ma_lpf_node lpfNode{};
        bool lpfEnabled = false;

        bool ready = false;
        bool groupsReady = false;

        std::unordered_map<SoundId, SoundDef *> sounds;
        std::vector<Voice *> activeVoices;

        SoundId nextSoundId = 1;
        Handle nextHandle = 1;
        Handle musicHandle = 0;

        float masterVolume = 1.0f;
        float sfxVolume = 1.0f;
        float musicVolume = 1.0f;
    };

    static Engine::Impl::SoundDef *findSound(Engine::Impl *impl, Engine::SoundId soundId, bool expectedMusic, bool checkType)
    {
        if (!impl || soundId <= 0)
            return nullptr;

        auto it = impl->sounds.find(soundId);
        if (it == impl->sounds.end())
            return nullptr;

        auto *sound = it->second;
        if (checkType && (sound->isMusic != expectedMusic))
            return nullptr;

        return sound;
    }

    static const Engine::Impl::Voice *findVoiceConst(const Engine::Impl *impl, Engine::Handle handle)
    {
        if (!impl || handle <= 0)
            return nullptr;

        for (const auto *voice : impl->activeVoices)
        {
            if (voice && voice->handle == handle)
                return voice;
        }

        return nullptr;
    }

    static Engine::Impl::Voice *findVoice(Engine::Impl *impl, Engine::Handle handle)
    {
        if (!impl || handle <= 0)
            return nullptr;

        for (auto *voice : impl->activeVoices)
        {
            if (voice && voice->handle == handle)
                return voice;
        }

        return nullptr;
    }

    static void destroyVoice(Engine::Impl *impl, Engine::Impl::Voice *voice)
    {
        if (!impl || !voice)
            return;

        if (impl->musicHandle == voice->handle)
            impl->musicHandle = 0;

        ma_sound_stop(&voice->sound);
        ma_sound_uninit(&voice->sound);
        delete voice;
    }

    static Engine::Handle createVoice(Engine::Impl *impl, Engine::Impl::SoundDef *soundDef, bool loop,
                                      float volume, float pitch, float pan)
    {
        if (!impl || !impl->ready || !soundDef)
            return 0;

        Engine::Impl::Voice *voice = new Engine::Impl::Voice();
        voice->handle = impl->nextHandle;
        voice->soundId = soundDef->id;
        voice->looping = loop;
        voice->isMusic = soundDef->isMusic;

        impl->nextHandle = impl->nextHandle + 1;
        if (impl->nextHandle <= 0)
            impl->nextHandle = 1;

        ma_sound_group *group = soundDef->isMusic ? &impl->musicGroup : &impl->sfxGroup;
        ma_result result;

        if (soundDef->procedural)
        {
            result = ma_sound_init_from_data_source(
                &impl->engine, soundDef->procedural->getDataSource(), 0, group, &voice->sound);
        }
        else
        {
            result = ma_sound_init_from_file(
                &impl->engine, soundDef->path.c_str(), 0, group, nullptr, &voice->sound);
        }

        if (result != MA_SUCCESS)
        {
            delete voice;
            return 0;
        }

        ma_sound_set_looping(&voice->sound, loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_volume(&voice->sound, clampRange(volume, 0.0f, 4.0f));
        ma_sound_set_pitch(&voice->sound, clampRange(pitch, kMinPitch, 4.0f));
        ma_sound_set_pan(&voice->sound, clampRange(pan, -1.0f, 1.0f));

        const ma_result startResult = ma_sound_start(&voice->sound);
        if (startResult != MA_SUCCESS)
        {
            ma_sound_uninit(&voice->sound);
            delete voice;
            return 0;
        }

        impl->activeVoices.push_back(voice);

        if (soundDef->isMusic)
            impl->musicHandle = voice->handle;

        return voice->handle;
    }

    Engine::Engine()
        : impl(new Impl())
    {
    }

    Engine::~Engine()
    {
        shutdown();
        delete impl;
        impl = nullptr;
    }

    bool Engine::init()
    {
        if (!impl)
            return false;
        if (impl->ready)
            return true;

        ma_engine_config config = ma_engine_config_init();
        const ma_result engineResult = ma_engine_init(&config, &impl->engine);
        if (engineResult != MA_SUCCESS)
            return false;

        const ma_result sfxResult = ma_sound_group_init(&impl->engine, 0, nullptr, &impl->sfxGroup);
        if (sfxResult != MA_SUCCESS)
        {
            ma_engine_uninit(&impl->engine);
            return false;
        }

        const ma_result musicResult = ma_sound_group_init(&impl->engine, 0, nullptr, &impl->musicGroup);
        if (musicResult != MA_SUCCESS)
        {
            ma_sound_group_uninit(&impl->sfxGroup);
            ma_engine_uninit(&impl->engine);
            return false;
        }

        // Initialize effects
        ma_delay_node_config delayConfig = ma_delay_node_config_init(ma_engine_get_channels(&impl->engine), ma_engine_get_sample_rate(&impl->engine), (ma_uint32)(ma_engine_get_sample_rate(&impl->engine) * 0.5f), 0.5f);
        if (ma_delay_node_init(ma_engine_get_node_graph(&impl->engine), &delayConfig, nullptr, &impl->delayNode) != MA_SUCCESS)
        {
            // Log error but continue
        }

        ma_lpf_node_config lpfConfig = ma_lpf_node_config_init(ma_engine_get_channels(&impl->engine), ma_engine_get_sample_rate(&impl->engine), 1000, 1);
        if (ma_lpf_node_init(ma_engine_get_node_graph(&impl->engine), &lpfConfig, nullptr, &impl->lpfNode) != MA_SUCCESS)
        {
            // Log error but continue
        }

        // Connect groups to endpoint by default
        ma_node_attach_output_bus(&impl->sfxGroup, 0, ma_engine_get_endpoint(&impl->engine), 0);
        ma_node_attach_output_bus(&impl->musicGroup, 0, ma_engine_get_endpoint(&impl->engine), 0);

        impl->groupsReady = true;
        impl->ready = true;

        ma_engine_set_volume(&impl->engine, clampRange(impl->masterVolume, 0.0f, 4.0f));
        ma_sound_group_set_volume(&impl->sfxGroup, clampRange(impl->sfxVolume, 0.0f, 4.0f));
        ma_sound_group_set_volume(&impl->musicGroup, clampRange(impl->musicVolume, 0.0f, 4.0f));

        return true;
    }

    void Engine::shutdown()
    {
        if (!impl)
            return;

        stopAll();
        clearSoundBank();

        if (!impl->ready)
            return;

        if (impl->groupsReady)
        {
            ma_delay_node_uninit(&impl->delayNode, nullptr);
            ma_lpf_node_uninit(&impl->lpfNode, nullptr);
            ma_sound_group_uninit(&impl->musicGroup);
            ma_sound_group_uninit(&impl->sfxGroup);
            impl->groupsReady = false;
        }

        ma_engine_uninit(&impl->engine);
        impl->ready = false;
    }

    bool Engine::isReady() const
    {
        return impl && impl->ready;
    }

    Engine::SoundId Engine::createSfx(const std::string &path)
    {
        if (!impl || path.empty())
            return 0;

        Impl::SoundDef *soundDef = new Impl::SoundDef();
        soundDef->id = impl->nextSoundId;
        soundDef->isMusic = false;
        soundDef->path = path;

        impl->nextSoundId = impl->nextSoundId + 1;
        if (impl->nextSoundId <= 0)
            impl->nextSoundId = 1;

        impl->sounds[soundDef->id] = soundDef;
        return soundDef->id;
    }

    Engine::SoundId Engine::createMusic(const std::string &path)
    {
        if (!impl || path.empty())
            return 0;

        Impl::SoundDef *soundDef = new Impl::SoundDef();
        soundDef->id = impl->nextSoundId;
        soundDef->isMusic = true;
        soundDef->path = path;

        impl->nextSoundId = impl->nextSoundId + 1;
        if (impl->nextSoundId <= 0)
            impl->nextSoundId = 1;

        impl->sounds[soundDef->id] = soundDef;
        return soundDef->id;
    }

    Engine::SoundId Engine::createWaveform(int type, float amplitude, float frequency)
    {
        if (!impl)
            return 0;

        Impl::SoundDef *soundDef = new Impl::SoundDef();
        soundDef->id = impl->nextSoundId++;
        soundDef->isMusic = false;
        // Use a default format of f32/stereo/48kHz for generated audio
        soundDef->procedural = new WaveformSource(
            ma_format_f32, 2, 48000, (ma_waveform_type)type, amplitude, frequency);

        if (impl->nextSoundId <= 0) impl->nextSoundId = 1;
        impl->sounds[soundDef->id] = soundDef;
        return soundDef->id;
    }

    Engine::SoundId Engine::createNoise(int type, int seed, float amplitude)
    {
        if (!impl)
            return 0;

        Impl::SoundDef *soundDef = new Impl::SoundDef();
        soundDef->id = impl->nextSoundId++;
        soundDef->isMusic = false;
        // Use a default format of f32/stereo for generated audio
        soundDef->procedural = new NoiseSource(
            ma_format_f32, 2, (ma_noise_type)type, seed, amplitude);

        if (impl->nextSoundId <= 0) impl->nextSoundId = 1;
        impl->sounds[soundDef->id] = soundDef;
        return soundDef->id;
    }

    bool Engine::removeSound(SoundId soundId)
    {
        if (!impl || soundId <= 0)
            return false;

        bool removed = false;

        for (auto it = impl->activeVoices.begin(); it != impl->activeVoices.end();)
        {
            Impl::Voice *voice = *it;
            if (voice && voice->soundId == soundId)
            {
                destroyVoice(impl, voice);
                it = impl->activeVoices.erase(it);
                removed = true;
                continue;
            }

            ++it;
        }

        auto it = impl->sounds.find(soundId);
        if (it != impl->sounds.end())
        {
            delete it->second;
            impl->sounds.erase(it);
            removed = true;
        }

        return removed;
    }

    void Engine::clearSoundBank()
    {
        if (!impl)
            return;

        for (auto &pair : impl->sounds)
        {
            auto *sound = pair.second;
            delete sound;
        }

        impl->sounds.clear();
    }

    Engine::Handle Engine::playSfx(SoundId soundId, float volume, float pitch, float pan)
    {
        Impl::SoundDef *soundDef = findSound(impl, soundId, false, true);
        if (!soundDef)
            return 0;

        return createVoice(impl, soundDef, false, volume, pitch, pan);
    }

    Engine::Handle Engine::playMusic(SoundId soundId, bool loop, float volume)
    {
        Impl::SoundDef *soundDef = findSound(impl, soundId, true, true);
        if (!soundDef)
            return 0;

        stopMusic();
        return createVoice(impl, soundDef, loop, volume, 1.0f, 0.0f);
    }

    void Engine::stopMusic()
    {
        if (!impl || impl->musicHandle <= 0)
            return;
        stop(impl->musicHandle);
    }

    bool Engine::isMusicPlaying() const
    {
        if (!impl || impl->musicHandle <= 0)
            return false;
        return isPlaying(impl->musicHandle);
    }

    bool Engine::stop(Handle handle)
    {
        if (!impl || handle <= 0)
            return false;

        for (auto it = impl->activeVoices.begin(); it != impl->activeVoices.end(); ++it)
        {
            Impl::Voice *voice = *it;
            if (!voice || voice->handle != handle)
                continue;

            destroyVoice(impl, voice);
            impl->activeVoices.erase(it);
            return true;
        }

        return false;
    }

    bool Engine::pause(Handle handle)
    {
        Impl::Voice *voice = findVoice(impl, handle);
        if (!voice)
            return false;

        return ma_sound_stop(&voice->sound) == MA_SUCCESS;
    }

    bool Engine::resume(Handle handle)
    {
        Impl::Voice *voice = findVoice(impl, handle);
        if (!voice)
            return false;

        return ma_sound_start(&voice->sound) == MA_SUCCESS;
    }

    bool Engine::setVolume(Handle handle, float volume)
    {
        Impl::Voice *voice = findVoice(impl, handle);
        if (!voice)
            return false;

        ma_sound_set_volume(&voice->sound, clampRange(volume, 0.0f, 4.0f));
        return true;
    }

    bool Engine::setPitch(Handle handle, float pitch)
    {
        Impl::Voice *voice = findVoice(impl, handle);
        if (!voice)
            return false;

        ma_sound_set_pitch(&voice->sound, clampRange(pitch, kMinPitch, 4.0f));
        return true;
    }

    bool Engine::setPan(Handle handle, float pan)
    {
        Impl::Voice *voice = findVoice(impl, handle);
        if (!voice)
            return false;

        ma_sound_set_pan(&voice->sound, clampRange(pan, -1.0f, 1.0f));
        return true;
    }

    bool Engine::isPlaying(Handle handle) const
    {
        const Impl::Voice *voice = findVoiceConst(impl, handle);
        if (!voice)
            return false;

        return ma_sound_is_playing(&voice->sound) == MA_TRUE;
    }

    void Engine::setMasterVolume(float volume)
    {
        if (!impl)
            return;

        impl->masterVolume = clampRange(volume, 0.0f, 4.0f);
        if (impl->ready)
            ma_engine_set_volume(&impl->engine, impl->masterVolume);
    }

    void Engine::setSfxVolume(float volume)
    {
        if (!impl)
            return;

        impl->sfxVolume = clampRange(volume, 0.0f, 4.0f);
        if (impl->ready && impl->groupsReady)
            ma_sound_group_set_volume(&impl->sfxGroup, impl->sfxVolume);
    }

    void Engine::setMusicVolume(float volume)
    {
        if (!impl)
            return;

        impl->musicVolume = clampRange(volume, 0.0f, 4.0f);
        if (impl->ready && impl->groupsReady)
            ma_sound_group_set_volume(&impl->musicGroup, impl->musicVolume);
    }

    void Engine::stopAll()
    {
        if (!impl)
            return;

        for (auto *voice : impl->activeVoices)
        {
            if (!voice)
                continue;
            destroyVoice(impl, voice);
        }

        impl->activeVoices.clear();
        impl->musicHandle = 0;
    }

    void Engine::update()
    {
        if (!impl || impl->activeVoices.empty())
            return;

        for (auto it = impl->activeVoices.begin(); it != impl->activeVoices.end();)
        {
            Impl::Voice *voice = *it;
            if (!voice)
            {
                it = impl->activeVoices.erase(it);
                continue;
            }

            if (!voice->looping && ma_sound_at_end(&voice->sound) == MA_TRUE)
            {
                destroyVoice(impl, voice);
                it = impl->activeVoices.erase(it);
                continue;
            }

            ++it;
        }
    }

    void Engine::enableSfxDelay(bool enable, float decay)
    {
        if (!impl || !impl->groupsReady) return;

        if (enable != impl->delayEnabled)
        {
            impl->delayEnabled = enable;
            if (enable)
            {
                ma_delay_node_set_decay(&impl->delayNode, decay);
                ma_node_attach_output_bus(&impl->sfxGroup, 0, &impl->delayNode, 0);
                ma_node_attach_output_bus(&impl->delayNode, 0, ma_engine_get_endpoint(&impl->engine), 0);
            }
            else
            {
                ma_node_attach_output_bus(&impl->sfxGroup, 0, ma_engine_get_endpoint(&impl->engine), 0);
            }
        }
    }

    void Engine::enableMusicLowPass(bool enable, float cutoff)
    {
        if (!impl || !impl->groupsReady) return;

        if (enable != impl->lpfEnabled)
        {
            impl->lpfEnabled = enable;
            if (enable)
            {
                ma_lpf_config config = ma_lpf_config_init(ma_format_f32, ma_engine_get_channels(&impl->engine), ma_engine_get_sample_rate(&impl->engine), cutoff, 1);
                ma_lpf_node_reinit(&config, &impl->lpfNode);
                ma_node_attach_output_bus(&impl->musicGroup, 0, &impl->lpfNode, 0);
                ma_node_attach_output_bus(&impl->lpfNode, 0, ma_engine_get_endpoint(&impl->engine), 0);
            }
            else
            {
                ma_node_attach_output_bus(&impl->musicGroup, 0, ma_engine_get_endpoint(&impl->engine), 0);
            }
        }
    }
}
