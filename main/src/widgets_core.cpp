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

    int SetNextWindowPos(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SetNextWindowPos()"))
            return 0;

        if (argCount < 2 || argCount > 5 || !args[0].isNumber() || !args[1].isNumber())
        {
            vm->runtimeError("ImGui.SetNextWindowPos expects (x, y[, cond[, pivotX, pivotY]])");
            return 0;
        }

        const float x = (float)args[0].asNumber();
        const float y = (float)args[1].asNumber();
        const ImGuiCond cond = (argCount >= 3 && args[2].isNumber()) ? (ImGuiCond)(int)args[2].asNumber() : ImGuiCond_None;

        if (argCount == 3)
        {
            ImGui::SetNextWindowPos(ImVec2(x, y), cond);
            return 0;
        }

        if (argCount == 5 && args[3].isNumber() && args[4].isNumber())
        {
            ImGui::SetNextWindowPos(ImVec2(x, y), cond, ImVec2((float)args[3].asNumber(), (float)args[4].asNumber()));
            return 0;
        }

        vm->runtimeError("ImGui.SetNextWindowPos expects (x, y[, cond[, pivotX, pivotY]])");
        return 0;
    }

    int SetNextWindowSize(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SetNextWindowSize()"))
            return 0;

        if (argCount < 2 || argCount > 3 || !args[0].isNumber() || !args[1].isNumber() || (argCount == 3 && !args[2].isNumber()))
        {
            vm->runtimeError("ImGui.SetNextWindowSize expects (width, height[, cond])");
            return 0;
        }

        const ImGuiCond cond = (argCount == 3) ? (ImGuiCond)(int)args[2].asNumber() : ImGuiCond_None;
        ImGui::SetNextWindowSize(ImVec2((float)args[0].asNumber(), (float)args[1].asNumber()), cond);
        return 0;
    }

    int BeginTabBar(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.BeginTabBar()"))
            return push_nil(vm);

        if ((argCount != 1 && argCount != 2) || !args[0].isString() || (argCount == 2 && !args[1].isNumber()))
        {
            vm->runtimeError("ImGui.BeginTabBar expects (id[, flags])");
            return push_nil(vm);
        }

        ImGuiTabBarFlags flags = (argCount == 2) ? (ImGuiTabBarFlags)(int)args[1].asNumber() : ImGuiTabBarFlags_None;
        vm->pushBool(ImGui::BeginTabBar(args[0].asStringChars(), flags));
        return 1;
    }

    int EndTabBar(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.EndTabBar()"))
            return 0;

        ImGui::EndTabBar();
        return 0;
    }

    int BeginTabItem(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.BeginTabItem()"))
            return push_nil(vm);

        if ((argCount != 1 && argCount != 2) || !args[0].isString() || (argCount == 2 && !args[1].isNumber()))
        {
            vm->runtimeError("ImGui.BeginTabItem expects (label[, flags])");
            return push_nil(vm);
        }

        ImGuiTabItemFlags flags = (argCount == 2) ? (ImGuiTabItemFlags)(int)args[1].asNumber() : ImGuiTabItemFlags_None;
        vm->pushBool(ImGui::BeginTabItem(args[0].asStringChars(), nullptr, flags));
        return 1;
    }

    int EndTabItem(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.EndTabItem()"))
            return 0;

        ImGui::EndTabItem();
        return 0;
    }

    int BeginDisabled(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.BeginDisabled()"))
            return 0;

        bool disabled = true;
        if (argCount == 1)
        {
            if (!args[0].isBool())
            {
                vm->runtimeError("ImGui.BeginDisabled expects () or (disabled)");
                return 0;
            }
            disabled = args[0].asBool();
        }
        else if (argCount != 0)
        {
            vm->runtimeError("ImGui.BeginDisabled expects () or (disabled)");
            return 0;
        }

        ImGui::BeginDisabled(disabled);
        return 0;
    }

    int EndDisabled(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.EndDisabled()"))
            return 0;

        ImGui::EndDisabled();
        return 0;
    }

    void register_core(ModuleBuilder &module)
    {
        module.addFunction("Init", Init, 0);
        module.addFunction("HasDemoWindow", HasDemoWindow, 0);
        module.addFunction("Begin", Begin, -1);
        module.addFunction("BeginChild", BeginChild, -1);
        module.addFunction("SetNextWindowPos", SetNextWindowPos, -1);
        module.addFunction("SetNextWindowSize", SetNextWindowSize, -1);
        module.addFunction("BeginTabBar", BeginTabBar, -1);
        module.addFunction("EndTabBar", EndTabBar, 0);
        module.addFunction("BeginTabItem", BeginTabItem, -1);
        module.addFunction("EndTabItem", EndTabItem, 0);
        module.addFunction("BeginDisabled", BeginDisabled, -1);
        module.addFunction("EndDisabled", EndDisabled, 0);
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
              .addInt("Cond_None", (int)ImGuiCond_None)
              .addInt("Cond_Always", (int)ImGuiCond_Always)
              .addInt("Cond_Once", (int)ImGuiCond_Once)
              .addInt("Cond_FirstUseEver", (int)ImGuiCond_FirstUseEver)
              .addInt("Cond_Appearing", (int)ImGuiCond_Appearing)
              .addInt("ChildFlags_None", (int)ImGuiChildFlags_None)
              .addInt("ChildFlags_Borders", (int)ImGuiChildFlags_Borders)
              .addInt("ChildFlags_AlwaysUseWindowPadding", (int)ImGuiChildFlags_AlwaysUseWindowPadding)
              .addInt("ChildFlags_ResizeX", (int)ImGuiChildFlags_ResizeX)
              .addInt("ChildFlags_ResizeY", (int)ImGuiChildFlags_ResizeY)
              .addInt("ChildFlags_AutoResizeX", (int)ImGuiChildFlags_AutoResizeX)
              .addInt("ChildFlags_AutoResizeY", (int)ImGuiChildFlags_AutoResizeY)
              .addInt("ChildFlags_AlwaysAutoResize", (int)ImGuiChildFlags_AlwaysAutoResize)
              .addInt("ChildFlags_FrameStyle", (int)ImGuiChildFlags_FrameStyle)
              .addInt("ChildFlags_NavFlattened", (int)ImGuiChildFlags_NavFlattened)
              .addInt("TabBarFlags_None", (int)ImGuiTabBarFlags_None)
              .addInt("TabBarFlags_Reorderable", (int)ImGuiTabBarFlags_Reorderable)
              .addInt("TabBarFlags_AutoSelectNewTabs", (int)ImGuiTabBarFlags_AutoSelectNewTabs)
              .addInt("TabBarFlags_FittingPolicyResizeDown", (int)ImGuiTabBarFlags_FittingPolicyResizeDown)
              .addInt("TabBarFlags_FittingPolicyScroll", (int)ImGuiTabBarFlags_FittingPolicyScroll)
              .addInt("TabItemFlags_None", (int)ImGuiTabItemFlags_None)
              .addInt("TabItemFlags_UnsavedDocument", (int)ImGuiTabItemFlags_UnsavedDocument)
              .addInt("TabItemFlags_SetSelected", (int)ImGuiTabItemFlags_SetSelected)
              .addInt("TabItemFlags_NoCloseWithMiddleMouseButton", (int)ImGuiTabItemFlags_NoCloseWithMiddleMouseButton)
              .addInt("TabItemFlags_NoTooltip", (int)ImGuiTabItemFlags_NoTooltip);
    }
}
