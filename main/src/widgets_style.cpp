#include "bindings_internal.hpp"

namespace ImGuiBindings
{
    int StyleColorsDark(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.StyleColorsDark()"))
            return 0;

        ImGui::StyleColorsDark();
        return 0;
    }

    int StyleColorsLight(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.StyleColorsLight()"))
            return 0;

        ImGui::StyleColorsLight();
        return 0;
    }

    int StyleColorsClassic(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.StyleColorsClassic()"))
            return 0;

        ImGui::StyleColorsClassic();
        return 0;
    }

    int PushStyleColor(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.PushStyleColor()"))
            return 0;

        if ((argCount != 4 && argCount != 5) || !args[0].isNumber() || !args[1].isNumber() ||
            !args[2].isNumber() || !args[3].isNumber() || (argCount == 5 && !args[4].isNumber()))
        {
            vm->runtimeError("ImGui.PushStyleColor expects (idx, r, g, b[, a])");
            return 0;
        }

        const float r = (float)args[1].asNumber();
        const float g = (float)args[2].asNumber();
        const float b = (float)args[3].asNumber();
        const float a = (argCount == 5) ? (float)args[4].asNumber() : 1.0f;
        ImGui::PushStyleColor((ImGuiCol)(int)args[0].asNumber(), ImVec4(r, g, b, a));
        return 0;
    }

    int PopStyleColor(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.PopStyleColor()"))
            return 0;

        if (argCount == 0)
        {
            ImGui::PopStyleColor();
            return 0;
        }

        if (argCount == 1 && args[0].isNumber())
        {
            ImGui::PopStyleColor((int)args[0].asNumber());
            return 0;
        }

        vm->runtimeError("ImGui.PopStyleColor expects () or (count)");
        return 0;
    }

    int SetStyleColor(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SetStyleColor()"))
            return push_nil(vm);

        if ((argCount != 4 && argCount != 5) || !args[0].isNumber() || !args[1].isNumber() ||
            !args[2].isNumber() || !args[3].isNumber() || (argCount == 5 && !args[4].isNumber()))
        {
            vm->runtimeError("ImGui.SetStyleColor expects (idx, r, g, b[, a])");
            return push_nil(vm);
        }

        const int idx = (int)args[0].asNumber();
        if (idx < 0 || idx >= ImGuiCol_COUNT)
        {
            vm->runtimeError("ImGui.SetStyleColor index out of range");
            return push_nil(vm);
        }

        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 &color = style.Colors[idx];
        color.x = (float)args[1].asNumber();
        color.y = (float)args[2].asNumber();
        color.z = (float)args[3].asNumber();
        color.w = (argCount == 5) ? (float)args[4].asNumber() : 1.0f;
        vm->pushBool(true);
        return 1;
    }

    int GetStyleColor(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.GetStyleColor()"))
            return push_nils(vm, 4);

        if (argCount != 1 || !args[0].isNumber())
        {
            vm->runtimeError("ImGui.GetStyleColor expects (idx)");
            return push_nils(vm, 4);
        }

        const int idx = (int)args[0].asNumber();
        if (idx < 0 || idx >= ImGuiCol_COUNT)
        {
            vm->runtimeError("ImGui.GetStyleColor index out of range");
            return push_nils(vm, 4);
        }

        const ImVec4 &color = ImGui::GetStyle().Colors[idx];
        vm->pushDouble(color.x);
        vm->pushDouble(color.y);
        vm->pushDouble(color.z);
        vm->pushDouble(color.w);
        return 4;
    }

    void register_style(ModuleBuilder &module)
    {
        module.addFunction("StyleColorsDark", StyleColorsDark, 0)
              .addFunction("StyleColorsLight", StyleColorsLight, 0)
              .addFunction("StyleColorsClassic", StyleColorsClassic, 0)
              .addFunction("PushStyleColor", PushStyleColor, -1)
              .addFunction("PopStyleColor", PopStyleColor, -1)
              .addFunction("SetStyleColor", SetStyleColor, -1)
              .addFunction("GetStyleColor", GetStyleColor, 1)
              .addInt("Col_Text", (int)ImGuiCol_Text)
              .addInt("Col_WindowBg", (int)ImGuiCol_WindowBg)
              .addInt("Col_ChildBg", (int)ImGuiCol_ChildBg)
              .addInt("Col_PopupBg", (int)ImGuiCol_PopupBg)
              .addInt("Col_Border", (int)ImGuiCol_Border)
              .addInt("Col_FrameBg", (int)ImGuiCol_FrameBg)
              .addInt("Col_FrameBgHovered", (int)ImGuiCol_FrameBgHovered)
              .addInt("Col_FrameBgActive", (int)ImGuiCol_FrameBgActive)
              .addInt("Col_TitleBg", (int)ImGuiCol_TitleBg)
              .addInt("Col_TitleBgActive", (int)ImGuiCol_TitleBgActive)
              .addInt("Col_Button", (int)ImGuiCol_Button)
              .addInt("Col_ButtonHovered", (int)ImGuiCol_ButtonHovered)
              .addInt("Col_ButtonActive", (int)ImGuiCol_ButtonActive)
              .addInt("Col_Header", (int)ImGuiCol_Header)
              .addInt("Col_HeaderHovered", (int)ImGuiCol_HeaderHovered)
              .addInt("Col_HeaderActive", (int)ImGuiCol_HeaderActive)
              .addInt("Col_Separator", (int)ImGuiCol_Separator)
              .addInt("Col_SeparatorHovered", (int)ImGuiCol_SeparatorHovered)
              .addInt("Col_SeparatorActive", (int)ImGuiCol_SeparatorActive)
              .addInt("Col_CheckMark", (int)ImGuiCol_CheckMark)
              .addInt("Col_SliderGrab", (int)ImGuiCol_SliderGrab)
              .addInt("Col_SliderGrabActive", (int)ImGuiCol_SliderGrabActive);
    }
}
