#include "bindings.hpp"
#include "device.hpp"
#include "input.hpp"

namespace SDLBindings
{
    static void sync_imgui_context_global(Interpreter *vm)
    {
        if (!vm)
            return;

        Value contextValue = Device::Instance().HasImGui()
                                 ? vm->makePointer(Device::Instance().GetImGuiContext())
                                 : vm->makeNil();

        if (!vm->setGlobal("__imgui_context", contextValue))
            vm->addGlobal("__imgui_context", contextValue);
    }

    static int native_Device_Init(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 && argc != 4)
        {
            Error("Device_Init expects (title, width, height[, flags])");
            return 0;
        }
        if (!args[0].isString() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("Device_Init expects (string, number, number[, number])");
            return 0;
        }

        const char *title = args[0].asStringChars();
        const int width = (int)args[1].asNumber();
        const int height = (int)args[2].asNumber();
        const unsigned int flags = (argc == 4) ? (unsigned int)args[3].asNumber() : 0u;

        const bool ok = Device::Instance().Init(title, width, height, flags);
        sync_imgui_context_global(vm);
        vm->pushBool(ok);
        return 1;
    }

    static int native_Device_InitImGui(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_InitImGui expects 0 arguments");
            return 0;
        }

        bool ok = Device::Instance().InitImGui();
        sync_imgui_context_global(vm);
        vm->pushBool(ok);
        return 1;
    }

    static int native_Device_ShutdownImGui(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_ShutdownImGui expects 0 arguments");
            return 0;
        }

        Device::Instance().ShutdownImGui();
        sync_imgui_context_global(vm);
        return 0;
    }

    static int native_Device_HasImGui(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_HasImGui expects 0 arguments");
            return 0;
        }

        vm->pushBool(Device::Instance().HasImGui());
        return 1;
    }

    static int native_Device_SetGLAttribute(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("SetGLAttribute expects (attr, value)");
            return 0;
        }
        if (Device::Instance().IsReady())
        {
            Error("SetGLAttribute must be called before Init");
            vm->pushBool(false);
            return 1;
        }

        SDL_GLattr attr = (SDL_GLattr)((int)args[0].asNumber());
        int value = (int)args[1].asNumber();
        int result = SDL_GL_SetAttribute(attr, value);
        vm->pushBool(result == 0);
        return 1;
    }

    static int native_Device_SetGLVersion(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("SetGLVersion expects (major, minor, profileMask)");
            return 0;
        }
        if (Device::Instance().IsReady())
        {
            Error("SetGLVersion must be called before Init");
            vm->pushBool(false);
            return 1;
        }

        int major = (int)args[0].asNumber();
        int minor = (int)args[1].asNumber();
        int profile = (int)args[2].asNumber();

        bool ok = true;
        ok = ok && (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major) == 0);
        ok = ok && (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor) == 0);
        ok = ok && (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile) == 0);

        vm->pushBool(ok);
        return 1;
    }

    static int native_Device_Close(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("Device_Close expects 0 arguments");
            return 0;
        }
        Device::Instance().Close();
        sync_imgui_context_global(vm);
        return 0;
    }

    static int native_Device_Running(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_Running expects 0 arguments");
            return 0;
        }
        vm->pushBool(Device::Instance().Running());
        return 1;
    }

    static int native_Device_Flip(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("Device_Flip expects 0 arguments");
            return 0;
        }
        Device::Instance().Flip();
        return 0;
    }

    static int native_Device_StartGifCapture(Interpreter *vm, int argc, Value *args)
    {
        if (argc > 1 || (argc == 1 && !args[0].isString()))
        {
            Error("Device_StartGifCapture expects ([path])");
            return 0;
        }

        const char *path = (argc == 1) ? args[0].asStringChars() : nullptr;
        vm->pushBool(Device::Instance().StartGifCapture(path));
        return 1;
    }

    static int native_Device_StopGifCapture(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_StopGifCapture expects 0 arguments");
            return 0;
        }

        vm->pushBool(Device::Instance().StopGifCapture());
        return 1;
    }

    static int native_Device_ToggleGifCapture(Interpreter *vm, int argc, Value *args)
    {
        if (argc > 1 || (argc == 1 && !args[0].isString()))
        {
            Error("Device_ToggleGifCapture expects ([path])");
            return 0;
        }

        const char *path = (argc == 1) ? args[0].asStringChars() : nullptr;
        vm->pushBool(Device::Instance().ToggleGifCapture(path));
        return 1;
    }

    static int native_Device_IsGifRecording(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_IsGifRecording expects 0 arguments");
            return 0;
        }

        vm->pushBool(Device::Instance().IsGifRecording());
        return 1;
    }

    static int native_Device_Quit(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("Device_Quit expects 0 arguments");
            return 0;
        }
        Device::Instance().Quit();
        return 0;
    }

    static int native_Device_SetTitle(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1 || !args[0].isString())
        {
            Error("Device_SetTitle expects 1 string argument");
            return 0;
        }
        Device::Instance().SetTitle(args[0].asStringChars());
        return 0;
    }

    static int native_Device_SetSize(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Device_SetSize expects (width, height)");
            return 0;
        }
        Device::Instance().SetSize((int)args[0].asNumber(), (int)args[1].asNumber());
        return 0;
    }

    static int native_Device_GetWindow(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_GetWindow expects 0 arguments");
            return 0;
        }
        vm->pushPointer(Device::Instance().GetWindow());
        return 1;
    }

    static int native_Device_GetContext(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_GetContext expects 0 arguments");
            return 0;
        }
        vm->pushPointer(Device::Instance().GetContext());
        return 1;
    }

    static int native_Device_GetWidth(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_GetWidth expects 0 arguments");
            return 0;
        }
        vm->pushInt(Device::Instance().GetWidth());
        return 1;
    }

    static int native_Device_GetHeight(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_GetHeight expects 0 arguments");
            return 0;
        }
        vm->pushInt(Device::Instance().GetHeight());
        return 1;
    }

    static int native_Device_GetDeltaTime(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_GetDeltaTime expects 0 arguments");
            return 0;
        }
        vm->pushDouble(Device::Instance().GetDeltaTime());
        return 1;
    }

    static int native_Device_GetFPS(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_GetFPS expects 0 arguments");
            return 0;
        }
        vm->pushInt((int)Device::Instance().GetFPS());
        return 1;
    }

    static int native_Device_IsResized(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_IsResized expects 0 arguments");
            return 0;
        }
        vm->pushBool(Device::Instance().IsResized());
        return 1;
    }

    static int native_Device_IsReady(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Device_IsReady expects 0 arguments");
            return 0;
        }
        vm->pushBool(Device::Instance().IsReady());
        return 1;
    }

    static int native_Input_IsMousePressed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsMousePressed expects 1 numeric button");
            return 0;
        }
        vm->pushBool(Input::IsMousePressed((MouseButton)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_IsMouseDown(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsMouseDown expects 1 numeric button");
            return 0;
        }
        vm->pushBool(Input::IsMouseDown((MouseButton)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_IsMouseReleased(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsMouseReleased expects 1 numeric button");
            return 0;
        }
        vm->pushBool(Input::IsMouseReleased((MouseButton)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_IsMouseUp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsMouseUp expects 1 numeric button");
            return 0;
        }
        vm->pushBool(Input::IsMouseUp((MouseButton)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_GetMousePosition(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetMousePosition expects 0 arguments");
            return 0;
        }
        Vec2 v = Input::GetMousePosition();
        vm->pushDouble(v.x);
        vm->pushDouble(v.y);
        return 2;
    }

    static int native_Input_GetMouseDelta(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetMouseDelta expects 0 arguments");
            return 0;
        }
        Vec2 v = Input::GetMouseDelta();
        vm->pushDouble(v.x);
        vm->pushDouble(v.y);
        return 2;
    }

    static int native_Input_GetMouseX(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetMouseX expects 0 arguments");
            return 0;
        }
        vm->pushInt(Input::GetMouseX());
        return 1;
    }

    static int native_Input_GetMouseY(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetMouseY expects 0 arguments");
            return 0;
        }
        vm->pushInt(Input::GetMouseY());
        return 1;
    }

    static int native_Input_SetMousePosition(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_SetMousePosition expects (x, y)");
            return 0;
        }
        Input::SetMousePosition((int)args[0].asNumber(), (int)args[1].asNumber());
        return 0;
    }

    static int native_Input_SetMouseRelative(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isBool())
        {
            Error("Input_SetMouseRelative expects 1 boolean");
            return 0;
        }
        vm->pushBool(Input::SetMouseRelative(args[0].asBool()));
        return 1;
    }

    static int native_Input_GetMouseRelative(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetMouseRelative expects 0 arguments");
            return 0;
        }
        vm->pushBool(Input::GetMouseRelative());
        return 1;
    }

    static int native_Input_SetMouseOffset(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_SetMouseOffset expects (offsetX, offsetY)");
            return 0;
        }
        Input::SetMouseOffset((int)args[0].asNumber(), (int)args[1].asNumber());
        return 0;
    }

    static int native_Input_SetMouseScale(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_SetMouseScale expects (scaleX, scaleY)");
            return 0;
        }
        Input::SetMouseScale((float)args[0].asNumber(), (float)args[1].asNumber());
        return 0;
    }

    static int native_Input_GetMouseWheelMove(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetMouseWheelMove expects 0 arguments");
            return 0;
        }
        Vec2 v = Input::GetMouseWheelMove();
        vm->pushDouble(v.x);
        vm->pushDouble(v.y);
        return 2;
    }

    static int native_Input_GetMouseWheelMoveV(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetMouseWheelMoveV expects 0 arguments");
            return 0;
        }
        vm->pushDouble(Input::GetMouseWheelMoveV());
        return 1;
    }

    static int native_Input_SetMouseCursor(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_SetMouseCursor expects 1 numeric cursor");
            return 0;
        }
        Input::SetMouseCursor((MouseCursor)((int)args[0].asNumber()));
        return 0;
    }

    static int native_Input_IsKeyPressed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsKeyPressed expects 1 numeric key");
            return 0;
        }
        vm->pushBool(Input::IsKeyPressed((KeyCode)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_IsKeyDown(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsKeyDown expects 1 numeric key");
            return 0;
        }
        vm->pushBool(Input::IsKeyDown((KeyCode)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_IsKeyReleased(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsKeyReleased expects 1 numeric key");
            return 0;
        }
        vm->pushBool(Input::IsKeyReleased((KeyCode)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_IsKeyUp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsKeyUp expects 1 numeric key");
            return 0;
        }
        vm->pushBool(Input::IsKeyUp((KeyCode)((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_GetKeyPressed(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetKeyPressed expects 0 arguments");
            return 0;
        }
        vm->pushInt((int)Input::GetKeyPressed());
        return 1;
    }

    static int native_Input_GetCharPressed(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetCharPressed expects 0 arguments");
            return 0;
        }
        vm->pushInt(Input::GetCharPressed());
        return 1;
    }

    static int native_Input_IsGamepadAvailable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_IsGamepadAvailable expects 1 numeric gamepad");
            return 0;
        }
        vm->pushBool(Input::IsGamepadAvailable((int)args[0].asNumber()));
        return 1;
    }

    static int native_Input_GetGamepadName(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_GetGamepadName expects 1 numeric gamepad");
            return 0;
        }
        vm->push(vm->makeString(Input::GetGamepadName((int)args[0].asNumber())));
        return 1;
    }

    static int native_Input_IsGamepadButtonPressed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_IsGamepadButtonPressed expects (gamepad, button)");
            return 0;
        }
        vm->pushBool(Input::IsGamepadButtonPressed((int)args[0].asNumber(),
                                                   (GamepadButton)((int)args[1].asNumber())));
        return 1;
    }

    static int native_Input_IsGamepadButtonDown(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_IsGamepadButtonDown expects (gamepad, button)");
            return 0;
        }
        vm->pushBool(Input::IsGamepadButtonDown((int)args[0].asNumber(),
                                                (GamepadButton)((int)args[1].asNumber())));
        return 1;
    }

    static int native_Input_IsGamepadButtonReleased(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_IsGamepadButtonReleased expects (gamepad, button)");
            return 0;
        }
        vm->pushBool(Input::IsGamepadButtonReleased((int)args[0].asNumber(),
                                                    (GamepadButton)((int)args[1].asNumber())));
        return 1;
    }

    static int native_Input_IsGamepadButtonUp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_IsGamepadButtonUp expects (gamepad, button)");
            return 0;
        }
        vm->pushBool(Input::IsGamepadButtonUp((int)args[0].asNumber(),
                                              (GamepadButton)((int)args[1].asNumber())));
        return 1;
    }

    static int native_Input_GetGamepadButtonPressed(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("Input_GetGamepadButtonPressed expects 0 arguments");
            return 0;
        }
        vm->pushInt(Input::GetGamepadButtonPressed());
        return 1;
    }

    static int native_Input_GetGamepadAxisCount(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("Input_GetGamepadAxisCount expects 1 numeric gamepad");
            return 0;
        }
        vm->pushInt(Input::GetGamepadAxisCount((int)args[0].asNumber()));
        return 1;
    }

    static int native_Input_GetGamepadAxisMovement(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Input_GetGamepadAxisMovement expects (gamepad, axis)");
            return 0;
        }
        vm->pushDouble(Input::GetGamepadAxisMovement((int)args[0].asNumber(),
                                                     (GamepadAxis)((int)args[1].asNumber())));
        return 1;
    }

    void register_device_input(ModuleBuilder &module)
    {
        module.addFunction("Init", native_Device_Init, -1)
            .addFunction("InitImGui", native_Device_InitImGui, 0)
            .addFunction("ShutdownImGui", native_Device_ShutdownImGui, 0)
            .addFunction("SetGLAttribute", native_Device_SetGLAttribute, 2)
            .addFunction("SetGLVersion", native_Device_SetGLVersion, 3)
            .addFunction("Quit", native_Device_Quit, 0)
            .addFunction("Running", native_Device_Running, 0)
            .addFunction("Flip", native_Device_Flip, 0)
            .addFunction("StartGifCapture", native_Device_StartGifCapture, -1)
            .addFunction("StopGifCapture", native_Device_StopGifCapture, 0)
            .addFunction("ToggleGifCapture", native_Device_ToggleGifCapture, -1)
            .addFunction("IsGifRecording", native_Device_IsGifRecording, 0)
            .addFunction("Close", native_Device_Close, 0)
            .addFunction("SetTitle", native_Device_SetTitle, 1)
            .addFunction("SetSize", native_Device_SetSize, 2)
            .addFunction("GetWindow", native_Device_GetWindow, 0)
            .addFunction("GetContext", native_Device_GetContext, 0)
            .addFunction("GetWidth", native_Device_GetWidth, 0)
            .addFunction("GetHeight", native_Device_GetHeight, 0)
            .addFunction("GetDeltaTime", native_Device_GetDeltaTime, 0)
            .addFunction("GetFPS", native_Device_GetFPS, 0)
            .addFunction("IsReady", native_Device_IsReady, 0)
            .addFunction("IsResized", native_Device_IsResized, 0)
            .addFunction("HasImGui", native_Device_HasImGui, 0)

            .addFunction("IsMousePressed", native_Input_IsMousePressed, 1)
            .addFunction("IsMouseDown", native_Input_IsMouseDown, 1)
            .addFunction("IsMouseReleased", native_Input_IsMouseReleased, 1)
            .addFunction("IsMouseUp", native_Input_IsMouseUp, 1)
            .addFunction("GetMousePosition", native_Input_GetMousePosition, 0)
            .addFunction("GetMouseDelta", native_Input_GetMouseDelta, 0)
            .addFunction("GetMouseX", native_Input_GetMouseX, 0)
            .addFunction("GetMouseY", native_Input_GetMouseY, 0)
            .addFunction("SetMousePosition", native_Input_SetMousePosition, 2)
            .addFunction("SetMouseRelative", native_Input_SetMouseRelative, 1)
            .addFunction("GetMouseRelative", native_Input_GetMouseRelative, 0)
            .addFunction("SetMouseOffset", native_Input_SetMouseOffset, 2)
            .addFunction("SetMouseScale", native_Input_SetMouseScale, 2)
            .addFunction("GetMouseWheelMove", native_Input_GetMouseWheelMove, 0)
            .addFunction("GetMouseWheelMoveV", native_Input_GetMouseWheelMoveV, 0)
            .addFunction("SetMouseCursor", native_Input_SetMouseCursor, 1)
            .addFunction("IsKeyPressed", native_Input_IsKeyPressed, 1)
            .addFunction("IsKeyDown", native_Input_IsKeyDown, 1)
            .addFunction("IsKeyReleased", native_Input_IsKeyReleased, 1)
            .addFunction("IsKeyUp", native_Input_IsKeyUp, 1)
            .addFunction("GetKeyPressed", native_Input_GetKeyPressed, 0)
            .addFunction("GetCharPressed", native_Input_GetCharPressed, 0)
            .addFunction("IsGamepadAvailable", native_Input_IsGamepadAvailable, 1)
            .addFunction("GetGamepadName", native_Input_GetGamepadName, 1)
            .addFunction("IsGamepadButtonPressed", native_Input_IsGamepadButtonPressed, 2)
            .addFunction("IsGamepadButtonDown", native_Input_IsGamepadButtonDown, 2)
            .addFunction("IsGamepadButtonReleased", native_Input_IsGamepadButtonReleased, 2)
            .addFunction("IsGamepadButtonUp", native_Input_IsGamepadButtonUp, 2)
            .addFunction("GetGamepadButtonPressed", native_Input_GetGamepadButtonPressed, 0)
            .addFunction("GetGamepadAxisCount", native_Input_GetGamepadAxisCount, 1)
            .addFunction("GetGamepadAxisMovement", native_Input_GetGamepadAxisMovement, 2);

        module.addInt("MOUSE_LEFT", LEFT)
            .addInt("MOUSE_RIGHT", RIGHT)
            .addInt("MOUSE_MIDDLE", MIDDLE)
            .addInt("MOUSE_CURSOR_DEFAULT", DEFAULT)
            .addInt("MOUSE_CURSOR_ARROW", ARROW)
            .addInt("MOUSE_CURSOR_IBEAM", IBEAM)
            .addInt("MOUSE_CURSOR_CROSSHAIR", CROSSHAIR)
            .addInt("MOUSE_CURSOR_POINTING_HAND", POINTING_HAND)
            .addInt("MOUSE_CURSOR_RESIZE_EW", RESIZE_EW)
            .addInt("MOUSE_CURSOR_RESIZE_NS", RESIZE_NS)
            .addInt("MOUSE_CURSOR_RESIZE_NWSE", RESIZE_NWSE)
            .addInt("MOUSE_CURSOR_RESIZE_NESW", RESIZE_NESW)
            .addInt("MOUSE_CURSOR_RESIZE_ALL", RESIZE_ALL)
            .addInt("MOUSE_CURSOR_NOT_ALLOWED", NOT_ALLOWED)
            .addInt("KEY_NULL", KEY_NULL)
            .addInt("KEY_APOSTROPHE", KEY_APOSTROPHE)
            .addInt("KEY_COMMA", KEY_COMMA)
            .addInt("KEY_MINUS", KEY_MINUS)
            .addInt("KEY_PERIOD", KEY_PERIOD)
            .addInt("KEY_SLASH", KEY_SLASH)
            .addInt("KEY_ZERO", KEY_ZERO)
            .addInt("KEY_1", KEY_ONE)
            .addInt("KEY_2", KEY_TWO)
            .addInt("KEY_3", KEY_THREE)
            .addInt("KEY_4", KEY_FOUR)
            .addInt("KEY_5", KEY_FIVE)
            .addInt("KEY_6", KEY_SIX)
            .addInt("KEY_7", KEY_SEVEN)
            .addInt("KEY_8", KEY_EIGHT)
            .addInt("KEY_9", KEY_NINE)
            .addInt("KEY_SEMICOLON", KEY_SEMICOLON)
            .addInt("KEY_EQUAL", KEY_EQUAL)
            .addInt("KEY_A", KEY_A)
            .addInt("KEY_B", KEY_B)
            .addInt("KEY_C", KEY_C)
            .addInt("KEY_D", KEY_D)
            .addInt("KEY_E", KEY_E)
            .addInt("KEY_F", KEY_F)
            .addInt("KEY_G", KEY_G)
            .addInt("KEY_H", KEY_H)
            .addInt("KEY_I", KEY_I)
            .addInt("KEY_J", KEY_J)
            .addInt("KEY_K", KEY_K)
            .addInt("KEY_L", KEY_L)
            .addInt("KEY_M", KEY_M)
            .addInt("KEY_N", KEY_N)
            .addInt("KEY_O", KEY_O)
            .addInt("KEY_P", KEY_P)
            .addInt("KEY_Q", KEY_Q)
            .addInt("KEY_R", KEY_R)
            .addInt("KEY_S", KEY_S)
            .addInt("KEY_T", KEY_T)
            .addInt("KEY_U", KEY_U)
            .addInt("KEY_V", KEY_V)
            .addInt("KEY_W", KEY_W)
            .addInt("KEY_X", KEY_X)
            .addInt("KEY_Y", KEY_Y)
            .addInt("KEY_Z", KEY_Z)
            .addInt("KEY_LEFT_BRACKET", KEY_LEFT_BRACKET)
            .addInt("KEY_BACKSLASH", KEY_BACKSLASH)
            .addInt("KEY_RIGHT_BRACKET", KEY_RIGHT_BRACKET)
            .addInt("KEY_GRAVE", KEY_GRAVE)
            .addInt("KEY_SPACE", KEY_SPACE)
            .addInt("KEY_ESCAPE", KEY_ESCAPE)
            .addInt("KEY_ENTER", KEY_ENTER)
            .addInt("KEY_TAB", KEY_TAB)
            .addInt("KEY_BACKSPACE", KEY_BACKSPACE)
            .addInt("KEY_INSERT", KEY_INSERT)
            .addInt("KEY_DELETE", KEY_DELETE)
            .addInt("KEY_RIGHT", KEY_RIGHT)
            .addInt("KEY_LEFT", KEY_LEFT)
            .addInt("KEY_DOWN", KEY_DOWN)
            .addInt("KEY_UP", KEY_UP)
            .addInt("KEY_PAGE_UP", KEY_PAGE_UP)
            .addInt("KEY_PAGE_DOWN", KEY_PAGE_DOWN)
            .addInt("KEY_HOME", KEY_HOME)
            .addInt("KEY_END", KEY_END)
            .addInt("KEY_CAPS_LOCK", KEY_CAPS_LOCK)
            .addInt("KEY_SCROLL_LOCK", KEY_SCROLL_LOCK)
            .addInt("KEY_NUM_LOCK", KEY_NUM_LOCK)
            .addInt("KEY_PRINT_SCREEN", KEY_PRINT_SCREEN)
            .addInt("KEY_PAUSE", KEY_PAUSE)
            .addInt("KEY_F1", KEY_F1)
            .addInt("KEY_F2", KEY_F2)
            .addInt("KEY_F3", KEY_F3)
            .addInt("KEY_F4", KEY_F4)
            .addInt("KEY_F5", KEY_F5)
            .addInt("KEY_F6", KEY_F6)
            .addInt("KEY_F7", KEY_F7)
            .addInt("KEY_F8", KEY_F8)
            .addInt("KEY_F9", KEY_F9)
            .addInt("KEY_F10", KEY_F10)
            .addInt("KEY_F11", KEY_F11)
            .addInt("KEY_F12", KEY_F12)
            .addInt("KEY_KP_0", KEY_KP_0)
            .addInt("KEY_KP_1", KEY_KP_1)
            .addInt("KEY_KP_2", KEY_KP_2)
            .addInt("KEY_KP_3", KEY_KP_3)
            .addInt("KEY_KP_4", KEY_KP_4)
            .addInt("KEY_KP_5", KEY_KP_5)
            .addInt("KEY_KP_6", KEY_KP_6)
            .addInt("KEY_KP_7", KEY_KP_7)
            .addInt("KEY_KP_8", KEY_KP_8)
            .addInt("KEY_KP_9", KEY_KP_9)
            .addInt("KEY_KP_DECIMAL", KEY_KP_DECIMAL)
            .addInt("KEY_KP_DIVIDE", KEY_KP_DIVIDE)
            .addInt("KEY_KP_MULTIPLY", KEY_KP_MULTIPLY)
            .addInt("KEY_KP_SUBTRACT", KEY_KP_SUBTRACT)
            .addInt("KEY_KP_ADD", KEY_KP_ADD)
            .addInt("KEY_KP_ENTER", KEY_KP_ENTER)
            .addInt("KEY_KP_EQUAL", KEY_KP_EQUAL)
            .addInt("KEY_LEFT_SHIFT", KEY_LEFT_SHIFT)
            .addInt("KEY_LEFT_CONTROL", KEY_LEFT_CONTROL)
            .addInt("KEY_LEFT_ALT", KEY_LEFT_ALT)
            .addInt("KEY_LEFT_SUPER", KEY_LEFT_SUPER)
            .addInt("KEY_RIGHT_SHIFT", KEY_RIGHT_SHIFT)
            .addInt("KEY_RIGHT_CONTROL", KEY_RIGHT_CONTROL)
            .addInt("KEY_RIGHT_ALT", KEY_RIGHT_ALT)
            .addInt("KEY_RIGHT_SUPER", KEY_RIGHT_SUPER)
            .addInt("KEY_KB_MENU", KEY_KB_MENU)
            .addInt("GAMEPAD_BUTTON_UNKNOWN", GAMEPAD_BUTTON_UNKNOWN)
            .addInt("GAMEPAD_BUTTON_LEFT_FACE_UP", GAMEPAD_BUTTON_LEFT_FACE_UP)
            .addInt("GAMEPAD_BUTTON_LEFT_FACE_RIGHT", GAMEPAD_BUTTON_LEFT_FACE_RIGHT)
            .addInt("GAMEPAD_BUTTON_LEFT_FACE_DOWN", GAMEPAD_BUTTON_LEFT_FACE_DOWN)
            .addInt("GAMEPAD_BUTTON_LEFT_FACE_LEFT", GAMEPAD_BUTTON_LEFT_FACE_LEFT)
            .addInt("GAMEPAD_BUTTON_RIGHT_FACE_UP", GAMEPAD_BUTTON_RIGHT_FACE_UP)
            .addInt("GAMEPAD_BUTTON_RIGHT_FACE_RIGHT", GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)
            .addInt("GAMEPAD_BUTTON_RIGHT_FACE_DOWN", GAMEPAD_BUTTON_RIGHT_FACE_DOWN)
            .addInt("GAMEPAD_BUTTON_RIGHT_FACE_LEFT", GAMEPAD_BUTTON_RIGHT_FACE_LEFT)
            .addInt("GAMEPAD_BUTTON_LEFT_TRIGGER_1", GAMEPAD_BUTTON_LEFT_TRIGGER_1)
            .addInt("GAMEPAD_BUTTON_LEFT_TRIGGER_2", GAMEPAD_BUTTON_LEFT_TRIGGER_2)
            .addInt("GAMEPAD_BUTTON_RIGHT_TRIGGER_1", GAMEPAD_BUTTON_RIGHT_TRIGGER_1)
            .addInt("GAMEPAD_BUTTON_RIGHT_TRIGGER_2", GAMEPAD_BUTTON_RIGHT_TRIGGER_2)
            .addInt("GAMEPAD_BUTTON_MIDDLE_LEFT", GAMEPAD_BUTTON_MIDDLE_LEFT)
            .addInt("GAMEPAD_BUTTON_MIDDLE", GAMEPAD_BUTTON_MIDDLE)
            .addInt("GAMEPAD_BUTTON_MIDDLE_RIGHT", GAMEPAD_BUTTON_MIDDLE_RIGHT)
            .addInt("GAMEPAD_BUTTON_LEFT_THUMB", GAMEPAD_BUTTON_LEFT_THUMB)
            .addInt("GAMEPAD_BUTTON_RIGHT_THUMB", GAMEPAD_BUTTON_RIGHT_THUMB)
            .addInt("GAMEPAD_AXIS_LEFT_X", GAMEPAD_AXIS_LEFT_X)
            .addInt("GAMEPAD_AXIS_LEFT_Y", GAMEPAD_AXIS_LEFT_Y)
            .addInt("GAMEPAD_AXIS_RIGHT_X", GAMEPAD_AXIS_RIGHT_X)
            .addInt("GAMEPAD_AXIS_RIGHT_Y", GAMEPAD_AXIS_RIGHT_Y)
            .addInt("GAMEPAD_AXIS_LEFT_TRIGGER", GAMEPAD_AXIS_LEFT_TRIGGER)
            .addInt("GAMEPAD_AXIS_RIGHT_TRIGGER", GAMEPAD_AXIS_RIGHT_TRIGGER);
    }
}
