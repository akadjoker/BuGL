#include "bindings.hpp"

namespace SDLBindings
{

    int native_SDL_CreateWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("SDL_CreateWindow expects 6 arguments");
            return 0;
        }
        if (!args[0].isString())
        {
            Error("SDL_CreateWindow expects string title");
            return 0;
        }

        const char *title = args[0].asStringChars();
        int x = args[1].asNumber();
        int y = args[2].asNumber();
        int w = args[3].asNumber();
        int h = args[4].asNumber();
        Uint32 flags = args[5].asNumber();

        SDL_Window *window = SDL_CreateWindow(title, x, y, w, h, flags);
        vm->push(vm->makePointer(window));
        return 1;
    }

    int native_SDL_DestroyWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_DestroyWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_DestroyWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_DestroyWindow(window);
        return 0;
    }

    int native_SDL_PollEvent(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_PollEvent expects 1 argument");
            return 0;
        }
        if (!args[0].isNativeStructInstance())
        {
            Error("SDL_PollEvent expects SDL_Event");
            return 0;
        }

        auto *eventInst = args[0].asNativeStructInstance();
        SDL_Event *event = (SDL_Event *)eventInst->data;

        int result = SDL_PollEvent(event);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_SetWindowTitle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetWindowTitle expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowTitle expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        const char *title = args[1].asStringChars();
        SDL_SetWindowTitle(window, title);
        return 0;
    }

    // =====================================================
    // EVENT FUNCTIONS
    // =====================================================

    int native_SDL_WaitEvent(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_WaitEvent expects 1 argument");
            return 0;
        }
        if (!args[0].isNativeStructInstance())
        {
            Error("SDL_WaitEvent expects SDL_Event");
            return 0;
        }

        auto *eventInst = args[0].asNativeStructInstance();
        SDL_Event *event = (SDL_Event *)eventInst->data;
        int result = SDL_WaitEvent(event);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_WaitEventTimeout(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_WaitEventTimeout expects 2 arguments");
            return 0;
        }
        if (!args[0].isNativeStructInstance())
        {
            Error("SDL_WaitEventTimeout expects SDL_Event");
            return 0;
        }

        auto *eventInst = args[0].asNativeStructInstance();
        SDL_Event *event = (SDL_Event *)eventInst->data;
        int timeout = args[1].asNumber();
        int result = SDL_WaitEventTimeout(event, timeout);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_FlushEvent(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_FlushEvent expects 1 argument");
            return 0;
        }
        Uint32 type = args[0].asNumber();
        SDL_FlushEvent(type);
        return 0;
    }

    int native_SDL_FlushEvents(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_FlushEvents expects 2 arguments");
            return 0;
        }
        Uint32 minType = args[0].asNumber();
        Uint32 maxType = args[1].asNumber();
        SDL_FlushEvents(minType, maxType);
        return 0;
    }

    int native_SDL_HasEvent(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_HasEvent expects 1 argument");
            return 0;
        }
        Uint32 type = args[0].asNumber();
        SDL_bool result = SDL_HasEvent(type);
        vm->push(vm->makeBool(result));
        return 1;
    }

    // =====================================================
    // KEYBOARD FUNCTIONS
    // =====================================================

    int native_SDL_GetKeyboardState(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetKeyboardState expects 1 argument (scancode)");
            return 0;
        }
        int scancode = args[0].asNumber();
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        vm->push(vm->makeInt(state[scancode]));
        return 1;
    }

    int native_SDL_GetScancodeFromKey(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetScancodeFromKey expects 1 argument");
            return 0;
        }
        SDL_Keycode key = args[0].asNumber();
        SDL_Scancode scancode = SDL_GetScancodeFromKey(key);
        vm->push(vm->makeInt(scancode));
        return 1;
    }

    int native_SDL_GetKeyFromScancode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetKeyFromScancode expects 1 argument");
            return 0;
        }
        SDL_Scancode scancode = (SDL_Scancode)args[0].asNumber();
        SDL_Keycode key = SDL_GetKeyFromScancode(scancode);
        vm->push(vm->makeInt(key));
        return 1;
    }

    int native_SDL_GetKeyName(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetKeyName expects 1 argument");
            return 0;
        }
        SDL_Keycode key = args[0].asNumber();
        const char *name = SDL_GetKeyName(key);
        vm->push(vm->makeString(name));
        return 1;
    }

    int native_SDL_GetModState(Interpreter *vm, int argc, Value *args)
    {
        SDL_Keymod mod = SDL_GetModState();
        vm->push(vm->makeInt(mod));
        return 1;
    }

    // =====================================================
    // MOUSE FUNCTIONS
    // =====================================================

    int native_SDL_GetMouseState(Interpreter *vm, int argc, Value *args)
    {
        int x, y;
        Uint32 buttons = SDL_GetMouseState(&x, &y);

        vm->push(vm->makeInt(x));
        vm->push(vm->makeInt(y));
        vm->push(vm->makeInt(buttons));
        return 3;
    }

    int native_SDL_GetGlobalMouseState(Interpreter *vm, int argc, Value *args)
    {
        int x, y;
        Uint32 buttons = SDL_GetGlobalMouseState(&x, &y);

        vm->push(vm->makeInt(x));
        vm->push(vm->makeInt(y));
        vm->push(vm->makeInt(buttons));
        return 3;
    }

    int native_SDL_GetRelativeMouseState(Interpreter *vm, int argc, Value *args)
    {
        int x, y;
        Uint32 buttons = SDL_GetRelativeMouseState(&x, &y);

        vm->push(vm->makeInt(x));
        vm->push(vm->makeInt(y));
        vm->push(vm->makeInt(buttons));
        return 3;
    }

    int native_SDL_WarpMouseInWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_WarpMouseInWindow expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_WarpMouseInWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int x = args[1].asNumber();
        int y = args[2].asNumber();
        SDL_WarpMouseInWindow(window, x, y);
        return 0;
    }

    int native_SDL_SetRelativeMouseMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_SetRelativeMouseMode expects 1 argument");
            return 0;
        }
        SDL_bool enabled = args[0].asBool() ? SDL_TRUE : SDL_FALSE;
        int result = SDL_SetRelativeMouseMode(enabled);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_GetRelativeMouseMode(Interpreter *vm, int argc, Value *args)
    {
        SDL_bool result = SDL_GetRelativeMouseMode();
        vm->push(vm->makeBool(result));
        return 1;
    }

    int native_SDL_ShowCursor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_ShowCursor expects 1 argument");
            return 0;
        }
        int toggle = args[0].asNumber();
        int result = SDL_ShowCursor(toggle);
        vm->push(vm->makeInt(result));
        return 1;
    }

    // =====================================================
    // WINDOW EXTRA FUNCTIONS
    // =====================================================

    int native_SDL_GetWindowSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowSize expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowSize expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        vm->push(vm->makeInt(w));
        vm->push(vm->makeInt(h));
        return 2;   
    }

    int native_SDL_SetWindowSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_SetWindowSize expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowSize expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int w = args[1].asNumber();
        int h = args[2].asNumber();
        SDL_SetWindowSize(window, w, h);
        return 0;
    }

    int native_SDL_GetWindowPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowPosition expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowPosition expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int x, y;
        SDL_GetWindowPosition(window, &x, &y);

        vm->push(vm->makeInt(x));
        vm->push(vm->makeInt(y));
        return 2;
    }

    int native_SDL_SetWindowPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_SetWindowPosition expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowPosition expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int x = args[1].asNumber();
        int y = args[2].asNumber();
        SDL_SetWindowPosition(window, x, y);
        return 0;
    }

    int native_SDL_ShowWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_ShowWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_ShowWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_ShowWindow(window);
        return 0;
    }

    int native_SDL_HideWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_HideWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_HideWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_HideWindow(window);
        return 0;
    }

    int native_SDL_RaiseWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RaiseWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RaiseWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_RaiseWindow(window);
        return 0;
    }

    int native_SDL_MaximizeWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_MaximizeWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_MaximizeWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_MaximizeWindow(window);
        return 0;
    }

    int native_SDL_MinimizeWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_MinimizeWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_MinimizeWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_MinimizeWindow(window);
        return 0;
    }

    int native_SDL_RestoreWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RestoreWindow expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RestoreWindow expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_RestoreWindow(window);
        return 0;
    }

    int native_SDL_SetWindowFullscreen(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetWindowFullscreen expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowFullscreen expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        Uint32 flags = args[1].asNumber();
        int result = SDL_SetWindowFullscreen(window, flags);
        vm->pushBool(result == 0);
        return 1;
    }

    int native_SDL_GetWindowFlags(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowFlags expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowFlags expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        Uint32 flags = SDL_GetWindowFlags(window);
        vm->pushDouble(flags);
        return 1;
    }

    int native_SDL_SetWindowGrab(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetWindowGrab expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowGrab expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_bool grabbed = args[1].asBool() ? SDL_TRUE : SDL_FALSE;
        SDL_SetWindowGrab(window, grabbed);
        return 0;
    }

    int native_SDL_GetWindowGrab(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowGrab expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowGrab expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_bool grabbed = SDL_GetWindowGrab(window);
        vm->pushBool(grabbed == SDL_TRUE);
        return 1;
    }

    // =====================================================
    // TIME FUNCTIONS
    // =====================================================

    int native_SDL_GetTicks(Interpreter *vm, int argc, Value *args)
    {
        Uint32 ticks = SDL_GetTicks();
        vm->pushDouble(ticks);
        return 1;
    }

    int native_SDL_GetPerformanceCounter(Interpreter *vm, int argc, Value *args)
    {
        Uint64 counter = SDL_GetPerformanceCounter();
        vm->pushDouble((double)counter);
        return 1;
    }

    int native_SDL_GetPerformanceFrequency(Interpreter *vm, int argc, Value *args)
    {
        Uint64 freq = SDL_GetPerformanceFrequency();
        vm->pushDouble((double)freq);
        return 1;
    }

    // =====================================================
    // WINDOW EXTRA FUNCTIONS
    // =====================================================

    int native_SDL_GetWindowID(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowID expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowID expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        Uint32 id = SDL_GetWindowID(window);
        vm->pushDouble(id);
        return 1;
    }

    int native_SDL_GetWindowFromID(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowFromID expects 1 argument");
            return 0;
        }

        Uint32 id = args[0].asNumber();
        SDL_Window *window = SDL_GetWindowFromID(id);
        if (window)
        {
            vm->pushPointer(window);
            return 1;
        }
        return 0;
    }

    int native_SDL_SetWindowBordered(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetWindowBordered expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowBordered expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_bool bordered = args[1].asBool() ? SDL_TRUE : SDL_FALSE;
        SDL_SetWindowBordered(window, bordered);
        return 0;
    }

    int native_SDL_SetWindowResizable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetWindowResizable expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowResizable expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        SDL_bool resizable = args[1].asBool() ? SDL_TRUE : SDL_FALSE;
        SDL_SetWindowResizable(window, resizable);
        return 0;
    }

    int native_SDL_SetWindowMinimumSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_SetWindowMinimumSize expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowMinimumSize expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int w = args[1].asNumber();
        int h = args[2].asNumber();
        SDL_SetWindowMinimumSize(window, w, h);
        return 0;
    }

    int native_SDL_SetWindowMaximumSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_SetWindowMaximumSize expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetWindowMaximumSize expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int w = args[1].asNumber();
        int h = args[2].asNumber();
        SDL_SetWindowMaximumSize(window, w, h);
        return 0;
    }

    int native_SDL_GetWindowMinimumSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowMinimumSize expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowMinimumSize expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int w, h;
        SDL_GetWindowMinimumSize(window, &w, &h);
        vm->pushInt(w);
        vm->pushInt(h);
        return 2;
    }

    int native_SDL_GetWindowMaximumSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowMaximumSize expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowMaximumSize expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int w, h;
        SDL_GetWindowMaximumSize(window, &w, &h);
        vm->pushInt(w);
        vm->pushInt(h);
        return 2;
    }

    int native_SDL_GetWindowTitle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowTitle expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowTitle expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        const char *title = SDL_GetWindowTitle(window);
        vm->pushString(title);
        return 1;
    }

    // =====================================================
    // DISPLAY FUNCTIONS
    // =====================================================

    int native_SDL_GetNumVideoDisplays(Interpreter *vm, int argc, Value *args)
    {
        int num = SDL_GetNumVideoDisplays();
        vm->pushInt(num);
        return 1;
    }

    int native_SDL_GetDisplayBounds(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetDisplayBounds expects 1 argument");
            return 0;
        }

        int displayIndex = args[0].asNumber();
        SDL_Rect rect;
        int result = SDL_GetDisplayBounds(displayIndex, &rect);

        if (result == 0)
        {
            vm->pushInt(rect.x);
            vm->pushInt(rect.y);
            vm->pushInt(rect.w);
            vm->pushInt(rect.h);
            return 4;
        }
        return 0;
    }

    int native_SDL_GetDisplayName(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetDisplayName expects 1 argument");
            return 0;
        }

        int displayIndex = args[0].asNumber();
        const char *name = SDL_GetDisplayName(displayIndex);
        vm->pushString(name);
        return 0;
    }

    int native_SDL_GetWindowDisplayIndex(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetWindowDisplayIndex expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetWindowDisplayIndex expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int index = SDL_GetWindowDisplayIndex(window);
        vm->pushInt(index);
        return 1;
    }

    // =====================================================
    // CLIPBOARD FUNCTIONS
    // =====================================================

    int native_SDL_SetClipboardText(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_SetClipboardText expects 1 argument");
            return 0;
        }
        if (!args[0].isString())
        {
            Error("SDL_SetClipboardText expects string");
            return 0;
        }

        const char *text = args[0].asStringChars();
        int result = SDL_SetClipboardText(text);
        vm->pushInt(result);
        return 1;
    }

    int native_SDL_GetClipboardText(Interpreter *vm, int argc, Value *args)
    {
        char *text = SDL_GetClipboardText();
        if (text)
        {
            Value result = vm->makeString(text);
            SDL_free(text);
            vm->push(result);
            return 1;
        }
        return 0;
    }

    int native_SDL_HasClipboardText(Interpreter *vm, int argc, Value *args)
    {
        SDL_bool result = SDL_HasClipboardText();
        vm->push(vm->makeBool(result));
        return 1;
    }

    // =====================================================
    // MESSAGE BOX FUNCTIONS
    // =====================================================

    int native_SDL_ShowSimpleMessageBox(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("SDL_ShowSimpleMessageBox expects 4 arguments (flags, title, message, window)");
            return 0;
        }

        Uint32 flags = args[0].asNumber();
        const char *title = args[1].asStringChars();
        const char *message = args[2].asStringChars();

        SDL_Window *window = nullptr;
        if (args[3].isPointer())
        {
            window = (SDL_Window *)args[3].asPointer();
        }

        int result = SDL_ShowSimpleMessageBox(flags, title, message, window);
        vm->pushInt(result);
        return 1;
    }

    void register_window(ModuleBuilder &mod)
    {

        // =============================================================
        //  WINDOW CREATION: Positioning
        // =============================================================
        mod.addInt("SDL_WINDOWPOS_CENTERED", SDL_WINDOWPOS_CENTERED)
            .addInt("SDL_WINDOWPOS_UNDEFINED", SDL_WINDOWPOS_UNDEFINED)

            // =============================================================
            //  WINDOW CREATION: Flags (Combináveis com + ou | no script)
            // =============================================================
            .addInt("SDL_WINDOW_RESIZABLE", SDL_WINDOW_RESIZABLE)
            .addInt("SDL_WINDOW_FULLSCREEN", SDL_WINDOW_FULLSCREEN)
            .addInt("SDL_WINDOW_FULLSCREEN_DESKTOP", SDL_WINDOW_FULLSCREEN_DESKTOP)
            .addInt("SDL_WINDOW_OPENGL", SDL_WINDOW_OPENGL)
            .addInt("SDL_WINDOW_VULKAN", SDL_WINDOW_VULKAN)
            .addInt("SDL_WINDOW_SHOWN", SDL_WINDOW_SHOWN)
            .addInt("SDL_WINDOW_HIDDEN", SDL_WINDOW_HIDDEN)
            .addInt("SDL_WINDOW_BORDERLESS", SDL_WINDOW_BORDERLESS)
            .addInt("SDL_WINDOW_ALLOW_HIGHDPI", SDL_WINDOW_ALLOW_HIGHDPI)
            // =============================================================
            // 5. EVENT TYPES (Para comparar com event.type)
            // =============================================================
            .addInt("SDL_QUIT", SDL_QUIT) // 256
            .addInt("SDL_KEYDOWN", SDL_KEYDOWN)  //0x300
            .addInt("SDL_KEYUP", SDL_KEYUP)      //0x301
            .addInt("SDL_MOUSEMOTION", SDL_MOUSEMOTION) //0x400
            .addInt("SDL_MOUSEBUTTONDOWN", SDL_MOUSEBUTTONDOWN) //0x402
            .addInt("SDL_MOUSEBUTTONUP", SDL_MOUSEBUTTONUP) //0x403
            .addInt("SDL_WINDOWEVENT", SDL_WINDOWEVENT)  //0x200
            .addInt("SDL_MOUSEWHEEL", SDL_MOUSEWHEEL)           // 1027

            // Mouse Buttons (Para comparar com event.button)
            .addByte("SDL_BUTTON_LEFT", SDL_BUTTON_LEFT)
            .addByte("SDL_BUTTON_MIDDLE", SDL_BUTTON_MIDDLE)
            .addByte("SDL_BUTTON_RIGHT", SDL_BUTTON_RIGHT)

        // =============================================================
        // 6. WINDOW EVENTS (Para comparar com event.winEvent)
        // =============================================================
        .addInt("SDL_WINDOWEVENT_SHOWN", SDL_WINDOWEVENT_SHOWN)
            .addInt("SDL_WINDOWEVENT_HIDDEN", SDL_WINDOWEVENT_HIDDEN)
            .addInt("SDL_WINDOWEVENT_EXPOSED", SDL_WINDOWEVENT_EXPOSED)
            .addInt("SDL_WINDOWEVENT_MOVED", SDL_WINDOWEVENT_MOVED)
            .addInt("SDL_WINDOWEVENT_RESIZED", SDL_WINDOWEVENT_RESIZED)
            .addInt("SDL_WINDOWEVENT_SIZE_CHANGED", SDL_WINDOWEVENT_SIZE_CHANGED)
            .addInt("SDL_WINDOWEVENT_MINIMIZED", SDL_WINDOWEVENT_MINIMIZED)
            .addInt("SDL_WINDOWEVENT_MAXIMIZED", SDL_WINDOWEVENT_MAXIMIZED)
            .addInt("SDL_WINDOWEVENT_RESTORED", SDL_WINDOWEVENT_RESTORED)
            .addInt("SDL_WINDOWEVENT_ENTER", SDL_WINDOWEVENT_ENTER)
            .addInt("SDL_WINDOWEVENT_LEAVE", SDL_WINDOWEVENT_LEAVE)
            .addInt("SDL_WINDOWEVENT_FOCUS_GAINED", SDL_WINDOWEVENT_FOCUS_GAINED)
            .addInt("SDL_WINDOWEVENT_FOCUS_LOST", SDL_WINDOWEVENT_FOCUS_LOST)
            .addInt("SDL_WINDOWEVENT_CLOSE", SDL_WINDOWEVENT_CLOSE)
            // Nota: Removi alguns menos usados como ICCPROF para poupar espaço,
            // mas podem ser adicionados futuramente

            // =============================================================
            // EVENT FUNCTIONS
            // =============================================================
            .addFunction("SDL_PollEvent", native_SDL_PollEvent, 1)
            .addFunction("SDL_WaitEvent", native_SDL_WaitEvent, 1)
            .addFunction("SDL_WaitEventTimeout", native_SDL_WaitEventTimeout, 2)
            .addFunction("SDL_FlushEvent", native_SDL_FlushEvent, 1)
            .addFunction("SDL_FlushEvents", native_SDL_FlushEvents, 2)
            .addFunction("SDL_HasEvent", native_SDL_HasEvent, 1)

            // =============================================================
            // KEYBOARD FUNCTIONS
            // =============================================================
            .addFunction("SDL_GetKeyboardState", native_SDL_GetKeyboardState, 1)
            .addFunction("SDL_GetScancodeFromKey", native_SDL_GetScancodeFromKey, 1)
            .addFunction("SDL_GetKeyFromScancode", native_SDL_GetKeyFromScancode, 1)
            .addFunction("SDL_GetKeyName", native_SDL_GetKeyName, 1)
            .addFunction("SDL_GetModState", native_SDL_GetModState, 0)

            // =============================================================
            // MOUSE FUNCTIONS
            // =============================================================
            .addFunction("SDL_GetMouseState", native_SDL_GetMouseState, 0)
            .addFunction("SDL_GetGlobalMouseState", native_SDL_GetGlobalMouseState, 0)
            .addFunction("SDL_GetRelativeMouseState", native_SDL_GetRelativeMouseState, 0)
            .addFunction("SDL_WarpMouseInWindow", native_SDL_WarpMouseInWindow, 3)
            .addFunction("SDL_SetRelativeMouseMode", native_SDL_SetRelativeMouseMode, 1)
            .addFunction("SDL_GetRelativeMouseMode", native_SDL_GetRelativeMouseMode, 0)
            .addFunction("SDL_ShowCursor", native_SDL_ShowCursor, 1)

            // =============================================================
            // WINDOW FUNCTIONS
            // =============================================================
            .addFunction("SDL_CreateWindow", native_SDL_CreateWindow, 6)
            .addFunction("SDL_DestroyWindow", native_SDL_DestroyWindow, 1)
            .addFunction("SDL_SetWindowTitle", native_SDL_SetWindowTitle, 2)
            .addFunction("SDL_GetWindowTitle", native_SDL_GetWindowTitle, 1)
            .addFunction("SDL_GetWindowSize", native_SDL_GetWindowSize, 1)
            .addFunction("SDL_SetWindowSize", native_SDL_SetWindowSize, 3)
            .addFunction("SDL_GetWindowPosition", native_SDL_GetWindowPosition, 1)
            .addFunction("SDL_SetWindowPosition", native_SDL_SetWindowPosition, 3)
            .addFunction("SDL_ShowWindow", native_SDL_ShowWindow, 1)
            .addFunction("SDL_HideWindow", native_SDL_HideWindow, 1)
            .addFunction("SDL_RaiseWindow", native_SDL_RaiseWindow, 1)
            .addFunction("SDL_MaximizeWindow", native_SDL_MaximizeWindow, 1)
            .addFunction("SDL_MinimizeWindow", native_SDL_MinimizeWindow, 1)
            .addFunction("SDL_RestoreWindow", native_SDL_RestoreWindow, 1)
            .addFunction("SDL_SetWindowFullscreen", native_SDL_SetWindowFullscreen, 2)
            .addFunction("SDL_GetWindowFlags", native_SDL_GetWindowFlags, 1)
            .addFunction("SDL_SetWindowGrab", native_SDL_SetWindowGrab, 2)
            .addFunction("SDL_GetWindowGrab", native_SDL_GetWindowGrab, 1)
            .addFunction("SDL_GetWindowID", native_SDL_GetWindowID, 1)
            .addFunction("SDL_GetWindowFromID", native_SDL_GetWindowFromID, 1)
            .addFunction("SDL_SetWindowBordered", native_SDL_SetWindowBordered, 2)
            .addFunction("SDL_SetWindowResizable", native_SDL_SetWindowResizable, 2)
            .addFunction("SDL_SetWindowMinimumSize", native_SDL_SetWindowMinimumSize, 3)
            .addFunction("SDL_SetWindowMaximumSize", native_SDL_SetWindowMaximumSize, 3)
            .addFunction("SDL_GetWindowMinimumSize", native_SDL_GetWindowMinimumSize, 1)
            .addFunction("SDL_GetWindowMaximumSize", native_SDL_GetWindowMaximumSize, 1)
            .addFunction("SDL_GetWindowDisplayIndex", native_SDL_GetWindowDisplayIndex, 1)

            // =============================================================
            // DISPLAY FUNCTIONS
            // =============================================================
            .addFunction("SDL_GetNumVideoDisplays", native_SDL_GetNumVideoDisplays, 0)
            .addFunction("SDL_GetDisplayBounds", native_SDL_GetDisplayBounds, 1)
            .addFunction("SDL_GetDisplayName", native_SDL_GetDisplayName, 1)

            // =============================================================
            // CLIPBOARD FUNCTIONS
            // =============================================================
            .addFunction("SDL_SetClipboardText", native_SDL_SetClipboardText, 1)
            .addFunction("SDL_GetClipboardText", native_SDL_GetClipboardText, 0)
            .addFunction("SDL_HasClipboardText", native_SDL_HasClipboardText, 0)

            // =============================================================
            // MESSAGE BOX FUNCTIONS
            // =============================================================
            .addFunction("SDL_ShowSimpleMessageBox", native_SDL_ShowSimpleMessageBox, 4)
            .addInt("SDL_MESSAGEBOX_ERROR", SDL_MESSAGEBOX_ERROR)
            .addInt("SDL_MESSAGEBOX_WARNING", SDL_MESSAGEBOX_WARNING)
            .addInt("SDL_MESSAGEBOX_INFORMATION", SDL_MESSAGEBOX_INFORMATION)

            // =============================================================
            // TIME FUNCTIONS
            // =============================================================
            .addFunction("SDL_GetTicks", native_SDL_GetTicks, 0)
            .addFunction("SDL_GetPerformanceCounter", native_SDL_GetPerformanceCounter, 0)
            .addFunction("SDL_GetPerformanceFrequency", native_SDL_GetPerformanceFrequency, 0)

            // =============================================================
            // KEYBOARD MODIFIERS
            // =============================================================
            .addInt("KMOD_NONE", KMOD_NONE)
            .addInt("KMOD_LSHIFT", KMOD_LSHIFT)
            .addInt("KMOD_RSHIFT", KMOD_RSHIFT)
            .addInt("KMOD_LCTRL", KMOD_LCTRL)
            .addInt("KMOD_RCTRL", KMOD_RCTRL)
            .addInt("KMOD_LALT", KMOD_LALT)
            .addInt("KMOD_RALT", KMOD_RALT)
            .addInt("KMOD_LGUI", KMOD_LGUI)
            .addInt("KMOD_RGUI", KMOD_RGUI)
            .addInt("KMOD_SHIFT", KMOD_SHIFT)
            .addInt("KMOD_CTRL", KMOD_CTRL)
            .addInt("KMOD_ALT", KMOD_ALT)
            .addInt("KMOD_GUI", KMOD_GUI)
            .addInt("KMOD_CAPS", KMOD_CAPS)
            .addInt("KMOD_NUM", KMOD_NUM)

            // =============================================================
            // KEYBOARD SCANCODES (Common ones)
            // =============================================================
            .addInt("SDL_SCANCODE_A", SDL_SCANCODE_A)
            .addInt("SDL_SCANCODE_B", SDL_SCANCODE_B)
            .addInt("SDL_SCANCODE_C", SDL_SCANCODE_C)
            .addInt("SDL_SCANCODE_D", SDL_SCANCODE_D)
            .addInt("SDL_SCANCODE_E", SDL_SCANCODE_E)
            .addInt("SDL_SCANCODE_F", SDL_SCANCODE_F)
            .addInt("SDL_SCANCODE_G", SDL_SCANCODE_G)
            .addInt("SDL_SCANCODE_H", SDL_SCANCODE_H)
            .addInt("SDL_SCANCODE_I", SDL_SCANCODE_I)
            .addInt("SDL_SCANCODE_J", SDL_SCANCODE_J)
            .addInt("SDL_SCANCODE_K", SDL_SCANCODE_K)
            .addInt("SDL_SCANCODE_L", SDL_SCANCODE_L)
            .addInt("SDL_SCANCODE_M", SDL_SCANCODE_M)
            .addInt("SDL_SCANCODE_N", SDL_SCANCODE_N)
            .addInt("SDL_SCANCODE_O", SDL_SCANCODE_O)
            .addInt("SDL_SCANCODE_P", SDL_SCANCODE_P)
            .addInt("SDL_SCANCODE_Q", SDL_SCANCODE_Q)
            .addInt("SDL_SCANCODE_R", SDL_SCANCODE_R)
            .addInt("SDL_SCANCODE_S", SDL_SCANCODE_S)
            .addInt("SDL_SCANCODE_T", SDL_SCANCODE_T)
            .addInt("SDL_SCANCODE_U", SDL_SCANCODE_U)
            .addInt("SDL_SCANCODE_V", SDL_SCANCODE_V)
            .addInt("SDL_SCANCODE_W", SDL_SCANCODE_W)
            .addInt("SDL_SCANCODE_X", SDL_SCANCODE_X)
            .addInt("SDL_SCANCODE_Y", SDL_SCANCODE_Y)
            .addInt("SDL_SCANCODE_Z", SDL_SCANCODE_Z)
            .addInt("SDL_SCANCODE_1", SDL_SCANCODE_1)
            .addInt("SDL_SCANCODE_2", SDL_SCANCODE_2)
            .addInt("SDL_SCANCODE_3", SDL_SCANCODE_3)
            .addInt("SDL_SCANCODE_4", SDL_SCANCODE_4)
            .addInt("SDL_SCANCODE_5", SDL_SCANCODE_5)
            .addInt("SDL_SCANCODE_6", SDL_SCANCODE_6)
            .addInt("SDL_SCANCODE_7", SDL_SCANCODE_7)
            .addInt("SDL_SCANCODE_8", SDL_SCANCODE_8)
            .addInt("SDL_SCANCODE_9", SDL_SCANCODE_9)
            .addInt("SDL_SCANCODE_0", SDL_SCANCODE_0)
            .addInt("SDL_SCANCODE_RETURN", SDL_SCANCODE_RETURN)
            .addInt("SDL_SCANCODE_ESCAPE", SDL_SCANCODE_ESCAPE)
            .addInt("SDL_SCANCODE_BACKSPACE", SDL_SCANCODE_BACKSPACE)
            .addInt("SDL_SCANCODE_TAB", SDL_SCANCODE_TAB)
            .addInt("SDL_SCANCODE_SPACE", SDL_SCANCODE_SPACE)
            .addInt("SDL_SCANCODE_F1", SDL_SCANCODE_F1)
            .addInt("SDL_SCANCODE_F2", SDL_SCANCODE_F2)
            .addInt("SDL_SCANCODE_F3", SDL_SCANCODE_F3)
            .addInt("SDL_SCANCODE_F4", SDL_SCANCODE_F4)
            .addInt("SDL_SCANCODE_F5", SDL_SCANCODE_F5)
            .addInt("SDL_SCANCODE_F6", SDL_SCANCODE_F6)
            .addInt("SDL_SCANCODE_F7", SDL_SCANCODE_F7)
            .addInt("SDL_SCANCODE_F8", SDL_SCANCODE_F8)
            .addInt("SDL_SCANCODE_F9", SDL_SCANCODE_F9)
            .addInt("SDL_SCANCODE_F10", SDL_SCANCODE_F10)
            .addInt("SDL_SCANCODE_F11", SDL_SCANCODE_F11)
            .addInt("SDL_SCANCODE_F12", SDL_SCANCODE_F12)
            .addInt("SDL_SCANCODE_RIGHT", SDL_SCANCODE_RIGHT)
            .addInt("SDL_SCANCODE_LEFT", SDL_SCANCODE_LEFT)
            .addInt("SDL_SCANCODE_DOWN", SDL_SCANCODE_DOWN)
            .addInt("SDL_SCANCODE_UP", SDL_SCANCODE_UP)
            .addInt("SDL_SCANCODE_LCTRL", SDL_SCANCODE_LCTRL)
            .addInt("SDL_SCANCODE_LSHIFT", SDL_SCANCODE_LSHIFT)
            .addInt("SDL_SCANCODE_LALT", SDL_SCANCODE_LALT)
            .addInt("SDL_SCANCODE_RCTRL", SDL_SCANCODE_RCTRL)
            .addInt("SDL_SCANCODE_RSHIFT", SDL_SCANCODE_RSHIFT)
            .addInt("SDL_SCANCODE_RALT", SDL_SCANCODE_RALT)

            // =============================================================
            // KEYCODES (SDLK_*) - Common ones
            // =============================================================
            .addInt("SDLK_a", SDLK_a)
            .addInt("SDLK_b", SDLK_b)
            .addInt("SDLK_c", SDLK_c)
            .addInt("SDLK_d", SDLK_d)
            .addInt("SDLK_e", SDLK_e)
            .addInt("SDLK_f", SDLK_f)
            .addInt("SDLK_g", SDLK_g)
            .addInt("SDLK_h", SDLK_h)
            .addInt("SDLK_i", SDLK_i)
            .addInt("SDLK_j", SDLK_j)
            .addInt("SDLK_k", SDLK_k)
            .addInt("SDLK_l", SDLK_l)
            .addInt("SDLK_m", SDLK_m)
            .addInt("SDLK_n", SDLK_n)
            .addInt("SDLK_o", SDLK_o)
            .addInt("SDLK_p", SDLK_p)
            .addInt("SDLK_q", SDLK_q)
            .addInt("SDLK_r", SDLK_r)
            .addInt("SDLK_s", SDLK_s)
            .addInt("SDLK_t", SDLK_t)
            .addInt("SDLK_u", SDLK_u)
            .addInt("SDLK_v", SDLK_v)
            .addInt("SDLK_w", SDLK_w)
            .addInt("SDLK_x", SDLK_x)
            .addInt("SDLK_y", SDLK_y)
            .addInt("SDLK_z", SDLK_z)
            .addInt("SDLK_0", SDLK_0)
            .addInt("SDLK_1", SDLK_1)
            .addInt("SDLK_2", SDLK_2)
            .addInt("SDLK_3", SDLK_3)
            .addInt("SDLK_4", SDLK_4)
            .addInt("SDLK_5", SDLK_5)
            .addInt("SDLK_6", SDLK_6)
            .addInt("SDLK_7", SDLK_7)
            .addInt("SDLK_8", SDLK_8)
            .addInt("SDLK_9", SDLK_9)
            .addInt("SDLK_RETURN", SDLK_RETURN)
            .addInt("SDLK_ESCAPE", SDLK_ESCAPE)
            .addInt("SDLK_BACKSPACE", SDLK_BACKSPACE)
            .addInt("SDLK_TAB", SDLK_TAB)
            .addInt("SDLK_SPACE", SDLK_SPACE)
            .addInt("SDLK_F1", SDLK_F1)
            .addInt("SDLK_F2", SDLK_F2)
            .addInt("SDLK_F3", SDLK_F3)
            .addInt("SDLK_F4", SDLK_F4)
            .addInt("SDLK_F5", SDLK_F5)
            .addInt("SDLK_F6", SDLK_F6)
            .addInt("SDLK_F7", SDLK_F7)
            .addInt("SDLK_F8", SDLK_F8)
            .addInt("SDLK_F9", SDLK_F9)
            .addInt("SDLK_F10", SDLK_F10)
            .addInt("SDLK_F11", SDLK_F11)
            .addInt("SDLK_F12", SDLK_F12)
            .addInt("SDLK_RIGHT", SDLK_RIGHT)
            .addInt("SDLK_LEFT", SDLK_LEFT)
            .addInt("SDLK_DOWN", SDLK_DOWN)
            .addInt("SDLK_UP", SDLK_UP)
            .addInt("SDLK_LCTRL", SDLK_LCTRL)
            .addInt("SDLK_LSHIFT", SDLK_LSHIFT)
            .addInt("SDLK_LALT", SDLK_LALT)
            .addInt("SDLK_RCTRL", SDLK_RCTRL)
            .addInt("SDLK_RSHIFT", SDLK_RSHIFT)
            .addInt("SDLK_RALT", SDLK_RALT)

            // =============================================================
            // CURSOR CONSTANTS
            // =============================================================
            .addInt("SDL_DISABLE", SDL_DISABLE)
            .addInt("SDL_ENABLE", SDL_ENABLE)
            .addInt("SDL_QUERY", SDL_QUERY);
    }

}