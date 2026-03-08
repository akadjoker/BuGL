#include "bindings.hpp"
#include "bugl_audio.hpp"

namespace SDLBindings
{
    namespace
    {
        bugl::audio::Engine &audio_engine()
        {
            static bugl::audio::Engine engine;
            return engine;
        }

        bool ensure_audio_ready()
        {
            bugl::audio::Engine &engine = audio_engine();
            if (engine.isReady())
                return true;
            return engine.init();
        }

        bool read_bool_like(const Value &value, bool *outValue)
        {
            if (!outValue)
                return false;

            if (value.isBool())
            {
                *outValue = value.asBool();
                return true;
            }

            if (value.isNumber())
            {
                *outValue = value.asNumber() != 0.0;
                return true;
            }

            return false;
        }
    }

    static int native_AudioInit(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("AudioInit expects 0 arguments");
            return 0;
        }

        vm->pushBool(audio_engine().init());
        return 1;
    }

    static int native_AudioClose(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("AudioClose expects 0 arguments");
            return 0;
        }

        audio_engine().shutdown();
        return 0;
    }

    static int native_AudioIsReady(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("AudioIsReady expects 0 arguments");
            return 0;
        }

        vm->pushBool(audio_engine().isReady());
        return 1;
    }

    static int native_AudioUpdate(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("AudioUpdate expects 0 arguments");
            return 0;
        }

        audio_engine().update();
        return 0;
    }

    static int native_AudioLoadSfx(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isString())
        {
            Error("AudioLoadSfx expects (path)");
            return 0;
        }

        vm->pushInt(audio_engine().createSfx(args[0].asStringChars()));
        return 1;
    }

    static int native_AudioLoadMusic(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isString())
        {
            Error("AudioLoadMusic expects (path)");
            return 0;
        }

        vm->pushInt(audio_engine().createMusic(args[0].asStringChars()));
        return 1;
    }

    static int native_AudioCreateWaveform(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("AudioCreateWaveform expects (type, amplitude, frequency)");
            return 0;
        }

        const int type = args[0].asInt();
        const float amplitude = (float)args[1].asNumber();
        const float frequency = (float)args[2].asNumber();

        vm->pushInt(audio_engine().createWaveform(type, amplitude, frequency));
        return 1;
    }

    static int native_AudioCreateNoise(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("AudioCreateNoise expects (type, seed, amplitude)");
            return 0;
        }

        const int type = args[0].asInt();
        const int seed = args[1].asInt();
        const float amplitude = (float)args[2].asNumber();

        vm->pushInt(audio_engine().createNoise(type, seed, amplitude));
        return 1;
    }

    static int native_AudioUnload(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioUnload expects (soundId)");
            return 0;
        }

        vm->pushBool(audio_engine().removeSound(args[0].asInt()));
        return 1;
    }

    static int native_AudioClearBank(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("AudioClearBank expects 0 arguments");
            return 0;
        }

        audio_engine().clearSoundBank();
        return 0;
    }

    static int native_AudioPlaySfx(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 1 || argc > 4 || !args[0].isNumber())
        {
            Error("AudioPlaySfx expects (soundId[, volume[, pitch[, pan]]])");
            return 0;
        }

        if (!ensure_audio_ready())
        {
            vm->pushInt(0);
            return 1;
        }

        float volume = 1.0f;
        float pitch = 1.0f;
        float pan = 0.0f;

        if (argc >= 2)
        {
            if (!args[1].isNumber())
            {
                Error("AudioPlaySfx volume must be numeric");
                return 0;
            }
            volume = (float)args[1].asNumber();
        }
        if (argc >= 3)
        {
            if (!args[2].isNumber())
            {
                Error("AudioPlaySfx pitch must be numeric");
                return 0;
            }
            pitch = (float)args[2].asNumber();
        }
        if (argc >= 4)
        {
            if (!args[3].isNumber())
            {
                Error("AudioPlaySfx pan must be numeric");
                return 0;
            }
            pan = (float)args[3].asNumber();
        }

        const int handle = audio_engine().playSfx(args[0].asInt(), volume, pitch, pan);
        vm->pushInt(handle);
        return 1;
    }

    static int native_AudioPlayMusic(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 1 || argc > 3 || !args[0].isNumber())
        {
            Error("AudioPlayMusic expects (soundId[, loop[, volume]])");
            return 0;
        }

        if (!ensure_audio_ready())
        {
            vm->pushInt(0);
            return 1;
        }

        bool loop = true;
        float volume = 1.0f;
        if (argc >= 2)
        {
            if (!read_bool_like(args[1], &loop))
            {
                Error("AudioPlayMusic loop must be bool or numeric");
                return 0;
            }
        }
        if (argc >= 3)
        {
            if (!args[2].isNumber())
            {
                Error("AudioPlayMusic volume must be numeric");
                return 0;
            }
            volume = (float)args[2].asNumber();
        }

        vm->pushInt(audio_engine().playMusic(args[0].asInt(), loop, volume));
        return 1;
    }

    static int native_AudioStopMusic(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("AudioStopMusic expects 0 arguments");
            return 0;
        }

        audio_engine().stopMusic();
        return 0;
    }

    static int native_AudioIsMusicPlaying(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("AudioIsMusicPlaying expects 0 arguments");
            return 0;
        }

        vm->pushBool(audio_engine().isMusicPlaying());
        return 1;
    }

    static int native_AudioStop(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioStop expects (handle)");
            return 0;
        }

        vm->pushBool(audio_engine().stop(args[0].asInt()));
        return 1;
    }

    static int native_AudioPause(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioPause expects (handle)");
            return 0;
        }

        vm->pushBool(audio_engine().pause(args[0].asInt()));
        return 1;
    }

    static int native_AudioResume(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioResume expects (handle)");
            return 0;
        }

        vm->pushBool(audio_engine().resume(args[0].asInt()));
        return 1;
    }

    static int native_AudioSetVolume(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("AudioSetVolume expects (handle, volume)");
            return 0;
        }

        vm->pushBool(audio_engine().setVolume(args[0].asInt(), (float)args[1].asNumber()));
        return 1;
    }

    static int native_AudioSetPitch(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("AudioSetPitch expects (handle, pitch)");
            return 0;
        }

        vm->pushBool(audio_engine().setPitch(args[0].asInt(), (float)args[1].asNumber()));
        return 1;
    }

    static int native_AudioSetPan(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("AudioSetPan expects (handle, pan)");
            return 0;
        }

        vm->pushBool(audio_engine().setPan(args[0].asInt(), (float)args[1].asNumber()));
        return 1;
    }

    static int native_AudioIsPlaying(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioIsPlaying expects (handle)");
            return 0;
        }

        vm->pushBool(audio_engine().isPlaying(args[0].asInt()));
        return 1;
    }

    static int native_AudioSetMasterVolume(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioSetMasterVolume expects (volume)");
            return 0;
        }

        audio_engine().setMasterVolume((float)args[0].asNumber());
        return 0;
    }

    static int native_AudioSetSfxVolume(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioSetSfxVolume expects (volume)");
            return 0;
        }

        audio_engine().setSfxVolume((float)args[0].asNumber());
        return 0;
    }

    static int native_AudioSetMusicVolume(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1 || !args[0].isNumber())
        {
            Error("AudioSetMusicVolume expects (volume)");
            return 0;
        }

        audio_engine().setMusicVolume((float)args[0].asNumber());
        return 0;
    }

    static int native_AudioEnableSfxDelay(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 1 || argc > 2)
        {
            Error("AudioEnableSfxDelay expects (enable[, decay])");
            return 0;
        }

        bool enable = false;
        if (!read_bool_like(args[0], &enable))
        {
            Error("AudioEnableSfxDelay enable must be bool or numeric");
            return 0;
        }

        float decay = 0.5f;
        if (argc == 2)
        {
            if (!args[1].isNumber())
            {
                Error("AudioEnableSfxDelay decay must be numeric");
                return 0;
            }
            decay = (float)args[1].asNumber();
        }

        audio_engine().enableSfxDelay(enable, decay);
        return 0;
    }

    static int native_AudioEnableMusicLowPass(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 1 || argc > 2)
        {
            Error("AudioEnableMusicLowPass expects (enable[, cutoff])");
            return 0;
        }

        bool enable = false;
        if (!read_bool_like(args[0], &enable))
        {
            Error("AudioEnableMusicLowPass enable must be bool or numeric");
            return 0;
        }

        float cutoff = 1000.0f;
        if (argc == 2)
        {
            if (!args[1].isNumber())
            {
                Error("AudioEnableMusicLowPass cutoff must be numeric");
                return 0;
            }
            cutoff = (float)args[1].asNumber();
        }

        audio_engine().enableMusicLowPass(enable, cutoff);
        return 0;
    }

    static int native_AudioStopAll(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("AudioStopAll expects 0 arguments");
            return 0;
        }

        audio_engine().stopAll();
        return 0;
    }

    void register_audio(ModuleBuilder &module)
    {
        module.addFunction("AudioInit", native_AudioInit, 0)
            .addFunction("AudioClose", native_AudioClose, 0)
            .addFunction("AudioIsReady", native_AudioIsReady, 0)
            .addFunction("AudioUpdate", native_AudioUpdate, 0)
            .addFunction("AudioLoadSfx", native_AudioLoadSfx, 1)
            .addFunction("AudioLoadMusic", native_AudioLoadMusic, 1)
            .addFunction("AudioCreateWaveform", native_AudioCreateWaveform, 3)
            .addFunction("AudioCreateNoise", native_AudioCreateNoise, 3)
            .addFunction("AudioUnload", native_AudioUnload, 1)
            .addFunction("AudioClearBank", native_AudioClearBank, 0)
            .addFunction("AudioPlaySfx", native_AudioPlaySfx, -1)
            .addFunction("AudioPlayMusic", native_AudioPlayMusic, -1)
            .addFunction("AudioStopMusic", native_AudioStopMusic, 0)
            .addFunction("AudioIsMusicPlaying", native_AudioIsMusicPlaying, 0)
            .addFunction("AudioStop", native_AudioStop, 1)
            .addFunction("AudioPause", native_AudioPause, 1)
            .addFunction("AudioResume", native_AudioResume, 1)
            .addFunction("AudioSetVolume", native_AudioSetVolume, 2)
            .addFunction("AudioSetPitch", native_AudioSetPitch, 2)
            .addFunction("AudioSetPan", native_AudioSetPan, 2)
            .addFunction("AudioIsPlaying", native_AudioIsPlaying, 1)
            .addFunction("AudioSetMasterVolume", native_AudioSetMasterVolume, 1)
            .addFunction("AudioSetSfxVolume", native_AudioSetSfxVolume, 1)
            .addFunction("AudioSetMusicVolume", native_AudioSetMusicVolume, 1)
            .addFunction("AudioEnableSfxDelay", native_AudioEnableSfxDelay, -1)
            .addFunction("AudioEnableMusicLowPass", native_AudioEnableMusicLowPass, -1)
            .addFunction("AudioStopAll", native_AudioStopAll, 0)
            .addInt("AUDIO_WAVE_SINE", bugl::audio::Engine::WAVE_SINE)
            .addInt("AUDIO_WAVE_SQUARE", bugl::audio::Engine::WAVE_SQUARE)
            .addInt("AUDIO_WAVE_TRIANGLE", bugl::audio::Engine::WAVE_TRIANGLE)
            .addInt("AUDIO_WAVE_SAW", bugl::audio::Engine::WAVE_SAW)
            .addInt("AUDIO_NOISE_WHITE", bugl::audio::Engine::NOISE_WHITE)
            .addInt("AUDIO_NOISE_PINK", bugl::audio::Engine::NOISE_PINK)
            .addInt("AUDIO_NOISE_BROWNIAN", bugl::audio::Engine::NOISE_BROWNIAN);
    }
}
