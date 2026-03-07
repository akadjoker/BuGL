#include "bindings.hpp"

namespace SDLBindings
{

    int native_SDL_Init(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_Init expects 1 argument");
            return 0;
        }

        Uint32 flags = args[0].asNumber();
        int result = SDL_Init(flags);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_Quit(Interpreter *vm, int argc, Value *args)
    {
        SDL_Quit();
        return 0;
    }

    int native_SDL_GetError(Interpreter *vm, int argc, Value *args)
    {
        const char *error = SDL_GetError();
         vm->push(vm->makeString(error));
        return 1;
    }

    int native_SDL_Delay(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_Delay expects 1 argument");
            return 0;
        }

        Uint32 ms = args[0].asNumber();
        SDL_Delay(ms);
        return 0;
    }

    void register_core(ModuleBuilder &mod)
    {

        mod.addFunction("SDL_Init", native_SDL_Init, 1)
            .addFunction("SDL_Quit", native_SDL_Quit, 0)
            .addFunction("SDL_GetError", native_SDL_GetError, 0)
            .addFunction("SDL_Delay", native_SDL_Delay, 1)

            // =============================================================
            // SYSTEM INITIALIZATION (Flags combináveis com | )
            // =============================================================
            .addInt("SDL_INIT_EVERYTHING", SDL_INIT_EVERYTHING)
            .addInt("SDL_INIT_VIDEO", SDL_INIT_VIDEO)
            .addInt("SDL_INIT_AUDIO", SDL_INIT_AUDIO)
            .addInt("SDL_INIT_TIMER", SDL_INIT_TIMER)
            .addInt("SDL_INIT_EVENTS", SDL_INIT_EVENTS)
            .addInt("SDL_INIT_JOYSTICK", SDL_INIT_JOYSTICK)
            .addInt("SDL_INIT_HAPTIC", SDL_INIT_HAPTIC)
            .addInt("SDL_INIT_GAMECONTROLLER", SDL_INIT_GAMECONTROLLER)
            .addInt("SDL_INIT_SENSOR", SDL_INIT_SENSOR);
    }
}