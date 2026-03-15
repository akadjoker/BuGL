#include "bindings_internal.hpp"

#include <string>
#include <vector>

namespace ImGuiBindings
{
    int SmallButton(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SmallButton()"))
            return push_nil(vm);

        if (argCount != 1 || !args[0].isString())
        {
            vm->runtimeError("ImGui.SmallButton expects (label)");
            return push_nil(vm);
        }

        vm->pushBool(ImGui::SmallButton(args[0].asStringChars()));
        return 1;
    }

    int Button(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Button()"))
            return push_nil(vm);

        if ((argCount != 1 && argCount != 3) || !args[0].isString() ||
            (argCount == 3 && (!args[1].isNumber() || !args[2].isNumber())))
        {
            vm->runtimeError("ImGui.Button expects (label) or (label, width, height)");
            return push_nil(vm);
        }

        ImVec2 size = ImVec2(0.0f, 0.0f);
        if (argCount == 3)
            size = ImVec2((float)args[1].asNumber(), (float)args[2].asNumber());

        vm->pushBool(ImGui::Button(args[0].asStringChars(), size));
        return 1;
    }

    int Selectable(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Selectable()"))
            return push_nils(vm, 2);

        if (argCount < 2 || argCount > 3 || !args[0].isString() || !args[1].isBool() || (argCount == 3 && !args[2].isNumber()))
        {
            vm->runtimeError("ImGui.Selectable expects (label, selected[, flags])");
            return push_nils(vm, 2);
        }

        bool selected = args[1].asBool();
        const ImGuiSelectableFlags flags = (argCount == 3) ? (ImGuiSelectableFlags)(int)args[2].asNumber() : ImGuiSelectableFlags_None;
        const bool changed = ImGui::Selectable(args[0].asStringChars(), &selected, flags);
        vm->pushBool(changed);
        vm->pushBool(selected);
        return 2;
    }

    int RadioButton(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.RadioButton()"))
            return push_nils(vm, 2);

        if (argCount != 2 || !args[0].isString() || !args[1].isBool())
        {
            vm->runtimeError("ImGui.RadioButton expects (label, active)");
            return push_nils(vm, 2);
        }

        bool active = args[1].asBool();
        const bool pressed = ImGui::RadioButton(args[0].asStringChars(), active);
        if (pressed)
            active = true;
        vm->pushBool(pressed);
        vm->pushBool(active);
        return 2;
    }

    int Checkbox(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Checkbox()"))
            return push_nils(vm, 2);

        if (argCount != 2 || !args[0].isString() || !args[1].isBool())
        {
            vm->runtimeError("ImGui.Checkbox expects (label, value)");
            return push_nils(vm, 2);
        }

        bool value = args[1].asBool();
        const bool changed = ImGui::Checkbox(args[0].asStringChars(), &value);
        vm->pushBool(changed);
        vm->pushBool(value);
        return 2;
    }

    int SliderFloat(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SliderFloat()"))
            return push_nils(vm, 2);

        if ((argCount != 4 && argCount != 5) || !args[0].isString() || !args[1].isNumber() ||
            !args[2].isNumber() || !args[3].isNumber() || (argCount == 5 && !args[4].isString()))
        {
            vm->runtimeError("ImGui.SliderFloat expects (label, value, min, max[, format])");
            return push_nils(vm, 2);
        }

        float value = (float)args[1].asNumber();
        const float minValue = (float)args[2].asNumber();
        const float maxValue = (float)args[3].asNumber();
        const char *format = (argCount == 5) ? args[4].asStringChars() : "%.3f";
        const bool changed = ImGui::SliderFloat(args[0].asStringChars(), &value, minValue, maxValue, format);
        vm->pushBool(changed);
        vm->pushDouble(value);
        return 2;
    }

    int SliderInt(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SliderInt()"))
            return push_nils(vm, 2);

        if (argCount < 4 || argCount > 6 || !args[0].isString() || !args[1].isNumber() ||
            !args[2].isNumber() || !args[3].isNumber())
        {
            vm->runtimeError("ImGui.SliderInt expects (label, value, min, max[, format[, flags]])");
            return push_nils(vm, 2);
        }

        int value = (int)args[1].asNumber();
        const int minValue = (int)args[2].asNumber();
        const int maxValue = (int)args[3].asNumber();
        const char *format = "%d";
        int flags = 0;
        if (!optional_string_arg(args, 4, argCount, "%d", &format) ||
            !optional_int_arg(args, 5, argCount, 0, &flags))
        {
            vm->runtimeError("ImGui.SliderInt expects (label, value, min, max[, format[, flags]])");
            return push_nils(vm, 2);
        }

        const bool changed = ImGui::SliderInt(args[0].asStringChars(), &value, minValue, maxValue, format, (ImGuiSliderFlags)flags);
        vm->pushBool(changed);
        vm->pushInt(value);
        return 2;
    }

    int DragFloat(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.DragFloat()"))
            return push_nils(vm, 2);

        if (argCount < 2 || argCount > 7 || !args[0].isString() || !args[1].isNumber())
        {
            vm->runtimeError("ImGui.DragFloat expects (label, value[, speed[, min[, max[, format[, flags]]]]])");
            return push_nils(vm, 2);
        }

        float value = (float)args[1].asNumber();
        float speed = 1.0f;
        float minValue = 0.0f;
        float maxValue = 0.0f;
        const char *format = "%.3f";
        int flags = 0;
        if (!optional_number_arg(args, 2, argCount, 1.0f, &speed) ||
            !optional_number_arg(args, 3, argCount, 0.0f, &minValue) ||
            !optional_number_arg(args, 4, argCount, 0.0f, &maxValue) ||
            !optional_string_arg(args, 5, argCount, "%.3f", &format) ||
            !optional_int_arg(args, 6, argCount, 0, &flags))
        {
            vm->runtimeError("ImGui.DragFloat expects (label, value[, speed[, min[, max[, format[, flags]]]]])");
            return push_nils(vm, 2);
        }

        const bool changed = ImGui::DragFloat(args[0].asStringChars(), &value, speed, minValue, maxValue, format, (ImGuiSliderFlags)flags);
        vm->pushBool(changed);
        vm->pushDouble(value);
        return 2;
    }

    int DragInt(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.DragInt()"))
            return push_nils(vm, 2);

        if (argCount < 2 || argCount > 7 || !args[0].isString() || !args[1].isNumber())
        {
            vm->runtimeError("ImGui.DragInt expects (label, value[, speed[, min[, max[, format[, flags]]]]])");
            return push_nils(vm, 2);
        }

        int value = (int)args[1].asNumber();
        float speed = 1.0f;
        int minValue = 0;
        int maxValue = 0;
        const char *format = "%d";
        int flags = 0;
        if (!optional_number_arg(args, 2, argCount, 1.0f, &speed) ||
            !optional_int_arg(args, 3, argCount, 0, &minValue) ||
            !optional_int_arg(args, 4, argCount, 0, &maxValue) ||
            !optional_string_arg(args, 5, argCount, "%d", &format) ||
            !optional_int_arg(args, 6, argCount, 0, &flags))
        {
            vm->runtimeError("ImGui.DragInt expects (label, value[, speed[, min[, max[, format[, flags]]]]])");
            return push_nils(vm, 2);
        }

        const bool changed = ImGui::DragInt(args[0].asStringChars(), &value, speed, minValue, maxValue, format, (ImGuiSliderFlags)flags);
        vm->pushBool(changed);
        vm->pushInt(value);
        return 2;
    }

    int InputFloat(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.InputFloat()"))
            return push_nils(vm, 2);

        if (argCount < 2 || argCount > 6 || !args[0].isString() || !args[1].isNumber())
        {
            vm->runtimeError("ImGui.InputFloat expects (label, value[, step[, stepFast[, format[, flags]]]])");
            return push_nils(vm, 2);
        }

        float value = (float)args[1].asNumber();
        float step = 0.0f;
        float stepFast = 0.0f;
        const char *format = "%.3f";
        int flags = 0;
        if (!optional_number_arg(args, 2, argCount, 0.0f, &step) ||
            !optional_number_arg(args, 3, argCount, 0.0f, &stepFast) ||
            !optional_string_arg(args, 4, argCount, "%.3f", &format) ||
            !optional_int_arg(args, 5, argCount, 0, &flags))
        {
            vm->runtimeError("ImGui.InputFloat expects (label, value[, step[, stepFast[, format[, flags]]]])");
            return push_nils(vm, 2);
        }

        const bool changed = ImGui::InputFloat(args[0].asStringChars(), &value, step, stepFast, format, flags);
        vm->pushBool(changed);
        vm->pushDouble(value);
        return 2;
    }

    int InputInt(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.InputInt()"))
            return push_nils(vm, 2);

        if (argCount < 2 || argCount > 5 || !args[0].isString() || !args[1].isNumber())
        {
            vm->runtimeError("ImGui.InputInt expects (label, value[, step[, stepFast[, flags]]])");
            return push_nils(vm, 2);
        }

        int value = (int)args[1].asNumber();
        int step = 1;
        int stepFast = 100;
        int flags = 0;
        if (!optional_int_arg(args, 2, argCount, 1, &step) ||
            !optional_int_arg(args, 3, argCount, 100, &stepFast) ||
            !optional_int_arg(args, 4, argCount, 0, &flags))
        {
            vm->runtimeError("ImGui.InputInt expects (label, value[, step[, stepFast[, flags]]])");
            return push_nils(vm, 2);
        }

        const bool changed = ImGui::InputInt(args[0].asStringChars(), &value, step, stepFast, flags);
        vm->pushBool(changed);
        vm->pushInt(value);
        return 2;
    }

    int InputText(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.InputText()"))
            return push_nils(vm, 2);

        if (argCount < 2 || argCount > 4 || !args[0].isString() || !args[1].isString())
        {
            vm->runtimeError("ImGui.InputText expects (label, value[, maxChars[, flags]])");
            return push_nils(vm, 2);
        }

        int maxChars = 256;
        int flags = 0;
        if (!optional_int_arg(args, 2, argCount, 256, &maxChars) ||
            !optional_int_arg(args, 3, argCount, 0, &flags))
        {
            vm->runtimeError("ImGui.InputText expects (label, value[, maxChars[, flags]])");
            return push_nils(vm, 2);
        }

        if (maxChars < 1)
            maxChars = 1;

        std::string text = args[1].asStringChars();
        if ((int)text.size() > maxChars)
            text.resize((size_t)maxChars);

        std::vector<char> buffer((size_t)maxChars + 1, '\0');
        for (size_t i = 0; i < text.size(); ++i)
            buffer[i] = text[i];

        const bool changed = ImGui::InputText(args[0].asStringChars(), buffer.data(), buffer.size(), (ImGuiInputTextFlags)flags);

        vm->pushBool(changed);
        vm->push(vm->makeString(buffer.data()));
        return 2;
    }

    int InputTextMultiline(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.InputTextMultiline()"))
            return push_nils(vm, 2);

        if (argCount < 2 || argCount > 6 || !args[0].isString() || !args[1].isString())
        {
            vm->runtimeError("ImGui.InputTextMultiline expects (label, value[, width[, height[, maxChars[, flags]]]])");
            return push_nils(vm, 2);
        }

        float width = 0.0f;
        float height = 0.0f;
        int maxChars = 512;
        int flags = 0;
        if (!optional_number_arg(args, 2, argCount, 0.0f, &width) ||
            !optional_number_arg(args, 3, argCount, 0.0f, &height) ||
            !optional_int_arg(args, 4, argCount, 512, &maxChars) ||
            !optional_int_arg(args, 5, argCount, 0, &flags))
        {
            vm->runtimeError("ImGui.InputTextMultiline expects (label, value[, width[, height[, maxChars[, flags]]]])");
            return push_nils(vm, 2);
        }

        if (maxChars < 1)
            maxChars = 1;

        std::string text = args[1].asStringChars();
        if ((int)text.size() > maxChars)
            text.resize((size_t)maxChars);

        std::vector<char> buffer((size_t)maxChars + 1, '\0');
        for (size_t i = 0; i < text.size(); ++i)
            buffer[i] = text[i];

        const bool changed = ImGui::InputTextMultiline(args[0].asStringChars(),
                                                       buffer.data(),
                                                       buffer.size(),
                                                       ImVec2(width, height),
                                                       (ImGuiInputTextFlags)flags);

        vm->pushBool(changed);
        vm->push(vm->makeString(buffer.data()));
        return 2;
    }

    int Combo(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Combo()"))
            return push_nils(vm, 2);

        if (argCount < 3 || !args[0].isString() || !args[1].isNumber())
        {
            vm->runtimeError("ImGui.Combo expects (label, currentIndex, item1, item2[, ...])");
            return push_nils(vm, 2);
        }

        std::vector<const char *> items;
        items.reserve((size_t)(argCount - 2));
        for (int i = 2; i < argCount; ++i)
        {
            if (!args[i].isString())
            {
                vm->runtimeError("ImGui.Combo expects only strings for items");
                return push_nils(vm, 2);
            }
            items.push_back(args[i].asStringChars());
        }

        int current = (int)args[1].asNumber();
        if (current < 0)
            current = 0;
        if (!items.empty() && current >= (int)items.size())
            current = (int)items.size() - 1;

        const bool changed = ImGui::Combo(args[0].asStringChars(), &current, items.data(), (int)items.size());
        vm->pushBool(changed);
        vm->pushInt(current);
        return 2;
    }

    int ColorEdit3(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.ColorEdit3()"))
            return push_nils(vm, 4);

        if ((argCount != 4 && argCount != 5) || !args[0].isString() || !args[1].isNumber() ||
            !args[2].isNumber() || !args[3].isNumber() || (argCount == 5 && !args[4].isNumber()))
        {
            vm->runtimeError("ImGui.ColorEdit3 expects (label, r, g, b[, flags])");
            return push_nils(vm, 4);
        }

        float color[3] = {
            (float)args[1].asNumber(),
            (float)args[2].asNumber(),
            (float)args[3].asNumber()
        };
        const ImGuiColorEditFlags flags = (argCount == 5) ? (ImGuiColorEditFlags)(int)args[4].asNumber() : ImGuiColorEditFlags_None;
        const bool changed = ImGui::ColorEdit3(args[0].asStringChars(), color, flags);
        vm->pushBool(changed);
        vm->pushDouble(color[0]);
        vm->pushDouble(color[1]);
        vm->pushDouble(color[2]);
        return 4;
    }

    int ColorEdit4(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.ColorEdit4()"))
            return push_nils(vm, 5);

        if ((argCount != 5 && argCount != 6) || !args[0].isString() || !args[1].isNumber() ||
            !args[2].isNumber() || !args[3].isNumber() || !args[4].isNumber() || (argCount == 6 && !args[5].isNumber()))
        {
            vm->runtimeError("ImGui.ColorEdit4 expects (label, r, g, b, a[, flags])");
            return push_nils(vm, 5);
        }

        float color[4] = {
            (float)args[1].asNumber(),
            (float)args[2].asNumber(),
            (float)args[3].asNumber(),
            (float)args[4].asNumber()
        };
        const ImGuiColorEditFlags flags = (argCount == 6) ? (ImGuiColorEditFlags)(int)args[5].asNumber() : ImGuiColorEditFlags_None;
        const bool changed = ImGui::ColorEdit4(args[0].asStringChars(), color, flags);
        vm->pushBool(changed);
        vm->pushDouble(color[0]);
        vm->pushDouble(color[1]);
        vm->pushDouble(color[2]);
        vm->pushDouble(color[3]);
        return 5;
    }

    int CollapsingHeader(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.CollapsingHeader()"))
            return push_nil(vm);

        if (argCount < 1 || argCount > 2 || !args[0].isString() || (argCount == 2 && !args[1].isNumber()))
        {
            vm->runtimeError("ImGui.CollapsingHeader expects (label[, flags])");
            return push_nil(vm);
        }

        ImGuiTreeNodeFlags flags = (argCount == 2) ? (ImGuiTreeNodeFlags)(int)args[1].asNumber() : ImGuiTreeNodeFlags_None;
        vm->pushBool(ImGui::CollapsingHeader(args[0].asStringChars(), flags));
        return 1;
    }

    int TreeNode(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.TreeNode()"))
            return push_nil(vm);

        if (argCount != 1 || !args[0].isString())
        {
            vm->runtimeError("ImGui.TreeNode expects (label)");
            return push_nil(vm);
        }

        vm->pushBool(ImGui::TreeNode(args[0].asStringChars()));
        return 1;
    }

    int TreePop(Interpreter *vm, int argCount, Value *args)
    {
        (void)args;
        if (!ensure_context(vm, "ImGui.TreePop()"))
            return 0;

        if (argCount != 0)
        {
            vm->runtimeError("ImGui.TreePop expects ()");
            return 0;
        }

        ImGui::TreePop();
        return 0;
    }

    int ProgressBar(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.ProgressBar()"))
            return 0;

        if (argCount < 1 || argCount > 4 || !args[0].isNumber())
        {
            vm->runtimeError("ImGui.ProgressBar expects (fraction[, width[, height[, overlay]]])");
            return 0;
        }

        float fraction = (float)args[0].asNumber();
        float width = -FLT_MIN;
        float height = 0.0f;
        const char *overlay = nullptr;

        if (!optional_number_arg(args, 1, argCount, -FLT_MIN, &width) ||
            !optional_number_arg(args, 2, argCount, 0.0f, &height) ||
            !optional_string_arg(args, 3, argCount, nullptr, &overlay))
        {
            vm->runtimeError("ImGui.ProgressBar expects (fraction[, width[, height[, overlay]]])");
            return 0;
        }

        ImGui::ProgressBar(fraction, ImVec2(width, height), overlay);
        return 0;
    }

    void register_inputs(ModuleBuilder &module)
    {
        module.addFunction("Button", Button, -1)
              .addFunction("SmallButton", SmallButton, 1)
              .addFunction("CollapsingHeader", CollapsingHeader, -1)
              .addFunction("TreeNode", TreeNode, 1)
              .addFunction("TreePop", TreePop, 0)
              .addFunction("ProgressBar", ProgressBar, -1)
              .addFunction("Selectable", Selectable, -1)
              .addFunction("RadioButton", RadioButton, 2)
              .addFunction("Checkbox", Checkbox, 2)
              .addFunction("DragFloat", DragFloat, -1)
              .addFunction("DragInt", DragInt, -1)
              .addFunction("SliderFloat", SliderFloat, -1)
              .addFunction("SliderInt", SliderInt, -1)
              .addFunction("InputFloat", InputFloat, -1)
              .addFunction("InputInt", InputInt, -1)
              .addFunction("InputText", InputText, -1)
              .addFunction("InputTextMultiline", InputTextMultiline, -1)
              .addFunction("Combo", Combo, -1)
              .addFunction("ColorEdit3", ColorEdit3, -1)
              .addFunction("ColorEdit4", ColorEdit4, -1)
              .addInt("SelectableFlags_None", (int)ImGuiSelectableFlags_None)
              .addInt("SelectableFlags_DontClosePopups", (int)ImGuiSelectableFlags_DontClosePopups)
              .addInt("SelectableFlags_SpanAllColumns", (int)ImGuiSelectableFlags_SpanAllColumns)
              .addInt("SelectableFlags_AllowDoubleClick", (int)ImGuiSelectableFlags_AllowDoubleClick)
              .addInt("SelectableFlags_Disabled", (int)ImGuiSelectableFlags_Disabled)
              .addInt("SelectableFlags_AllowOverlap", (int)ImGuiSelectableFlags_AllowOverlap)
              .addInt("SelectableFlags_Highlight", (int)ImGuiSelectableFlags_Highlight)
              .addInt("SelectableFlags_SelectOnNav", (int)ImGuiSelectableFlags_SelectOnNav)
              .addInt("SliderFlags_None", (int)ImGuiSliderFlags_None)
              .addInt("SliderFlags_AlwaysClamp", (int)ImGuiSliderFlags_AlwaysClamp)
              .addInt("SliderFlags_Logarithmic", (int)ImGuiSliderFlags_Logarithmic)
              .addInt("SliderFlags_NoRoundToFormat", (int)ImGuiSliderFlags_NoRoundToFormat)
              .addInt("SliderFlags_NoInput", (int)ImGuiSliderFlags_NoInput)
              .addInt("InputTextFlags_None", (int)ImGuiInputTextFlags_None)
              .addInt("InputTextFlags_CharsDecimal", (int)ImGuiInputTextFlags_CharsDecimal)
              .addInt("InputTextFlags_CharsHexadecimal", (int)ImGuiInputTextFlags_CharsHexadecimal)
              .addInt("InputTextFlags_CharsUppercase", (int)ImGuiInputTextFlags_CharsUppercase)
              .addInt("InputTextFlags_CharsNoBlank", (int)ImGuiInputTextFlags_CharsNoBlank)
              .addInt("InputTextFlags_AutoSelectAll", (int)ImGuiInputTextFlags_AutoSelectAll)
              .addInt("InputTextFlags_EnterReturnsTrue", (int)ImGuiInputTextFlags_EnterReturnsTrue)
              .addInt("InputTextFlags_ReadOnly", (int)ImGuiInputTextFlags_ReadOnly)
              .addInt("InputTextFlags_Password", (int)ImGuiInputTextFlags_Password)
              .addInt("ColorEdit_None", (int)ImGuiColorEditFlags_None)
              .addInt("ColorEdit_NoAlpha", (int)ImGuiColorEditFlags_NoAlpha)
              .addInt("ColorEdit_NoPicker", (int)ImGuiColorEditFlags_NoPicker)
              .addInt("ColorEdit_NoOptions", (int)ImGuiColorEditFlags_NoOptions)
              .addInt("ColorEdit_NoSmallPreview", (int)ImGuiColorEditFlags_NoSmallPreview)
              .addInt("ColorEdit_NoInputs", (int)ImGuiColorEditFlags_NoInputs)
              .addInt("ColorEdit_NoTooltip", (int)ImGuiColorEditFlags_NoTooltip)
              .addInt("ColorEdit_NoLabel", (int)ImGuiColorEditFlags_NoLabel)
              .addInt("ColorEdit_AlphaBar", (int)ImGuiColorEditFlags_AlphaBar)
              .addInt("ColorEdit_DisplayRGB", (int)ImGuiColorEditFlags_DisplayRGB)
              .addInt("ColorEdit_DisplayHSV", (int)ImGuiColorEditFlags_DisplayHSV)
              .addInt("ColorEdit_DisplayHex", (int)ImGuiColorEditFlags_DisplayHex)
              .addInt("ColorEdit_Uint8", (int)ImGuiColorEditFlags_Uint8)
              .addInt("ColorEdit_Float", (int)ImGuiColorEditFlags_Float)
              .addInt("ColorEdit_PickerHueBar", (int)ImGuiColorEditFlags_PickerHueBar)
              .addInt("ColorEdit_PickerHueWheel", (int)ImGuiColorEditFlags_PickerHueWheel)
              .addInt("ColorEdit_InputRGB", (int)ImGuiColorEditFlags_InputRGB)
              .addInt("ColorEdit_InputHSV", (int)ImGuiColorEditFlags_InputHSV);
    }
}
