#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <SDL2/SDL_opengl.h>
 

namespace SDLBindings
{
    // =====================================================
    // SDL_GL CONTEXT FUNCTIONS
    // =====================================================

    int native_SDL_GL_CreateContext(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GL_CreateContext expects 1 argument");
             vm->pushNil();
            return 1;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GL_CreateContext expects window pointer");
            vm->pushNil();
            return 1;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_GLContext context = SDL_GL_CreateContext(window);
        if (context)
            vm->pushPointer(context);
        else
            vm->pushNil();
        return 1;
    }

    int native_SDL_GL_DeleteContext(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GL_DeleteContext expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GL_DeleteContext expects context pointer");
            return 0;
        }

        SDL_GLContext context = (SDL_GLContext)args[0].asPointer();
        SDL_GL_DeleteContext(context);
        return 0;
    }

    int native_SDL_GL_MakeCurrent(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_GL_MakeCurrent expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer() || !args[1].isPointer())
        {
            Error("SDL_GL_MakeCurrent expects window and context pointers");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_GLContext context = (SDL_GLContext)args[1].asPointer();
        int result = SDL_GL_MakeCurrent(window, context);
        vm->pushInt(result);
        return 1;
    }

    int native_SDL_GL_SwapWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GL_SwapWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GL_SwapWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_GL_SwapWindow(window);
        return 0;
    }

    int native_SDL_GL_SetAttribute(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_GL_SetAttribute expects 2 arguments");
            return 0;
        }

        SDL_GLattr attr = (SDL_GLattr)args[0].asNumber();
        int value = args[1].asNumber();
        int result = SDL_GL_SetAttribute(attr, value);
        vm->pushInt(result);
        return 1;
    }

    int native_SDL_GL_GetAttribute(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GL_GetAttribute expects 1 argument");
            return 0;
        }

        SDL_GLattr attr = (SDL_GLattr)args[0].asNumber();
        int value = 0;
        int result = SDL_GL_GetAttribute(attr, &value);
        vm->push(vm->makeInt(value));
        return 1;
    }

    int native_SDL_GL_SetSwapInterval(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GL_SetSwapInterval expects 1 argument");
            return 0;
        }

        int interval = args[0].asNumber();
        int result = SDL_GL_SetSwapInterval(interval);
        vm->pushInt(result);
        return 1;
    }

    int native_SDL_GL_GetSwapInterval(Interpreter *vm, int argc, Value *args)
    {
        int interval = SDL_GL_GetSwapInterval();
        vm->push(vm->makeInt(interval));
        return 1;
    }

    int native_SDL_GL_GetCurrentContext(Interpreter *vm, int argc, Value *args)
    {
        SDL_GLContext context = SDL_GL_GetCurrentContext();
        if (context)
            vm->push(vm->makePointer(context));
        else
            vm->push(vm->makeNil());
        return 1;
    }

    int native_SDL_GL_GetCurrentWindow(Interpreter *vm, int argc, Value *args)
    {
        SDL_Window *window = SDL_GL_GetCurrentWindow();
        if (window)
            vm->push(vm->makePointer(window));
        else
            vm->push(vm->makeNil());
        return 1;
    }

   
    void register_sdl_opengl(ModuleBuilder &module)
    {
        // =============================================================
        // SDL_GL CONTEXT FUNCTIONS
        // =============================================================
        module.addFunction("SDL_GL_CreateContext", native_SDL_GL_CreateContext, 1)
            .addFunction("SDL_GL_DeleteContext", native_SDL_GL_DeleteContext, 1)
            .addFunction("SDL_GL_MakeCurrent", native_SDL_GL_MakeCurrent, 2)
            .addFunction("SDL_GL_SwapWindow", native_SDL_GL_SwapWindow, 1)
            .addFunction("SDL_GL_SetAttribute", native_SDL_GL_SetAttribute, 2)
            .addFunction("SDL_GL_GetAttribute", native_SDL_GL_GetAttribute, 1)
            .addFunction("SDL_GL_SetSwapInterval", native_SDL_GL_SetSwapInterval, 1)
            .addFunction("SDL_GL_GetSwapInterval", native_SDL_GL_GetSwapInterval, 0)
            .addFunction("SDL_GL_GetCurrentContext", native_SDL_GL_GetCurrentContext, 0)
            .addFunction("SDL_GL_GetCurrentWindow", native_SDL_GL_GetCurrentWindow, 0)
      
            // =============================================================
            // SDL_GL ATTRIBUTES
            // =============================================================
            .addInt("SDL_GL_RED_SIZE", SDL_GL_RED_SIZE)
            .addInt("SDL_GL_GREEN_SIZE", SDL_GL_GREEN_SIZE)
            .addInt("SDL_GL_BLUE_SIZE", SDL_GL_BLUE_SIZE)
            .addInt("SDL_GL_ALPHA_SIZE", SDL_GL_ALPHA_SIZE)
            .addInt("SDL_GL_BUFFER_SIZE", SDL_GL_BUFFER_SIZE)
            .addInt("SDL_GL_DOUBLEBUFFER", SDL_GL_DOUBLEBUFFER)
            .addInt("SDL_GL_DEPTH_SIZE", SDL_GL_DEPTH_SIZE)
            .addInt("SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE)
            .addInt("SDL_GL_CONTEXT_MAJOR_VERSION", SDL_GL_CONTEXT_MAJOR_VERSION)
            .addInt("SDL_GL_CONTEXT_MINOR_VERSION", SDL_GL_CONTEXT_MINOR_VERSION)
            .addInt("SDL_GL_CONTEXT_PROFILE_MASK", SDL_GL_CONTEXT_PROFILE_MASK)
            .addInt("SDL_GL_CONTEXT_PROFILE_CORE", SDL_GL_CONTEXT_PROFILE_CORE)
            .addInt("SDL_GL_CONTEXT_PROFILE_COMPATIBILITY", SDL_GL_CONTEXT_PROFILE_COMPATIBILITY)
            .addInt("SDL_GL_CONTEXT_PROFILE_ES", SDL_GL_CONTEXT_PROFILE_ES)
            .addInt("SDL_GL_MULTISAMPLEBUFFERS", SDL_GL_MULTISAMPLEBUFFERS)
            .addInt("SDL_GL_MULTISAMPLESAMPLES", SDL_GL_MULTISAMPLESAMPLES);

    }
}
