#include "bindings_internal.hpp"

namespace ImGuiBindings
{
    int HasDemoWindow(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
#ifdef IMGUI_DISABLE_DEMO_WINDOWS
        vm->pushBool(false);
#else
        vm->pushBool(true);
#endif
        return 1;
    }

    int Init(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!sync_context(vm, "ImGui.Init()"))
            return 0;
        return 0;
    }

    int Begin(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Begin()"))
            return push_nil(vm);

        if ((argCount != 1 && argCount != 2) || !args[0].isString() || (argCount == 2 && !args[1].isNumber()))
        {
            vm->runtimeError("ImGui.Begin expects (name[, flags])");
            return push_nil(vm);
        }

        const ImGuiWindowFlags flags = (argCount == 2) ? (ImGuiWindowFlags)(int)args[1].asNumber() : ImGuiWindowFlags_None;
        const bool collapsed = ImGui::Begin(args[0].asStringChars(), nullptr, flags);
        vm->push(vm->makeBool(collapsed));
        return 1;
    }

    int BeginChild(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.BeginChild()"))
            return push_nil(vm);

        if (argCount < 3 || argCount > 5 || !args[0].isString() || !args[1].isNumber() || !args[2].isNumber())
        {
            vm->runtimeError("ImGui.BeginChild expects (name, width, height[, childFlags[, windowFlags]])");
            return push_nil(vm);
        }

        const ImGuiChildFlags childFlags = (argCount >= 4) ? (ImGuiChildFlags)(int)args[3].asNumber() : ImGuiChildFlags_None;
        const ImGuiWindowFlags windowFlags = (argCount >= 5) ? (ImGuiWindowFlags)(int)args[4].asNumber() : ImGuiWindowFlags_None;
        const bool visible = ImGui::BeginChild(args[0].asStringChars(),
                                               ImVec2((float)args[1].asNumber(), (float)args[2].asNumber()),
                                               childFlags,
                                               windowFlags);
        vm->pushBool(visible);
        return 1;
    }

    int EndChild(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.EndChild()"))
            return 0;

        ImGui::EndChild();
        return 0;
    }

    int End(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.End()"))
            return 0;

        ImGui::End();
        return 0;
    }

    int ShowDemoWindow(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.ShowDemoWindow()"))
            return 0;

#ifdef IMGUI_DISABLE_DEMO_WINDOWS
        vm->runtimeError("ImGui.ShowDemoWindow() is disabled in this build (BUGL_IMGUI_DEMO=OFF)");
        return 0;
#else
        ImGui::ShowDemoWindow();
#endif
        return 0;
    }

    void register_core(ModuleBuilder &module)
    {
        module.addFunction("Init", Init, 0);
        module.addFunction("HasDemoWindow", HasDemoWindow, 0);
        module.addFunction("Begin", Begin, -1);
        module.addFunction("BeginChild", BeginChild, -1);
        module.addFunction("End", End, 0);
        module.addFunction("EndChild", EndChild, 0);
        module.addFunction("ShowDemoWindow", ShowDemoWindow, 0);
        module.addInt("WindowFlags_None", (int)ImGuiWindowFlags_None)
              .addInt("WindowFlags_NoTitleBar", (int)ImGuiWindowFlags_NoTitleBar)
              .addInt("WindowFlags_NoResize", (int)ImGuiWindowFlags_NoResize)
              .addInt("WindowFlags_NoMove", (int)ImGuiWindowFlags_NoMove)
              .addInt("WindowFlags_NoScrollbar", (int)ImGuiWindowFlags_NoScrollbar)
              .addInt("WindowFlags_NoScrollWithMouse", (int)ImGuiWindowFlags_NoScrollWithMouse)
              .addInt("WindowFlags_NoCollapse", (int)ImGuiWindowFlags_NoCollapse)
              .addInt("WindowFlags_AlwaysAutoResize", (int)ImGuiWindowFlags_AlwaysAutoResize)
              .addInt("WindowFlags_NoBackground", (int)ImGuiWindowFlags_NoBackground)
              .addInt("WindowFlags_NoSavedSettings", (int)ImGuiWindowFlags_NoSavedSettings)
              .addInt("WindowFlags_HorizontalScrollbar", (int)ImGuiWindowFlags_HorizontalScrollbar)
              .addInt("WindowFlags_AlwaysVerticalScrollbar", (int)ImGuiWindowFlags_AlwaysVerticalScrollbar)
              .addInt("WindowFlags_AlwaysHorizontalScrollbar", (int)ImGuiWindowFlags_AlwaysHorizontalScrollbar)
              .addInt("ChildFlags_None", (int)ImGuiChildFlags_None)
              .addInt("ChildFlags_Borders", (int)ImGuiChildFlags_Borders)
              .addInt("ChildFlags_AlwaysUseWindowPadding", (int)ImGuiChildFlags_AlwaysUseWindowPadding)
              .addInt("ChildFlags_ResizeX", (int)ImGuiChildFlags_ResizeX)
              .addInt("ChildFlags_ResizeY", (int)ImGuiChildFlags_ResizeY)
              .addInt("ChildFlags_AutoResizeX", (int)ImGuiChildFlags_AutoResizeX)
              .addInt("ChildFlags_AutoResizeY", (int)ImGuiChildFlags_AutoResizeY)
              .addInt("ChildFlags_AlwaysAutoResize", (int)ImGuiChildFlags_AlwaysAutoResize)
              .addInt("ChildFlags_FrameStyle", (int)ImGuiChildFlags_FrameStyle)
              .addInt("ChildFlags_NavFlattened", (int)ImGuiChildFlags_NavFlattened);
    }
}
