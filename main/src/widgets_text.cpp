#include "bindings_internal.hpp"

namespace ImGuiBindings
{
    int Text(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Text()"))
            return 0;

        if (argCount < 1 || !args[0].isString())
        {
            vm->runtimeError("ImGui.Text expects (text)");
            return 0;
        }

        ImGui::TextUnformatted(args[0].asStringChars());

        return 0;
    }

    int TextColored(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.TextColored()"))
            return 0;

        if ((argCount != 4 && argCount != 5) || !args[0].isNumber() || !args[1].isNumber() ||
            !args[2].isNumber() || (argCount == 4 ? !args[3].isString() : (!args[3].isNumber() || !args[4].isString())))
        {
            vm->runtimeError("ImGui.TextColored expects (r, g, b, text) or (r, g, b, a, text)");
            return 0;
        }

        float r = (float)args[0].asNumber();
        float g = (float)args[1].asNumber();
        float b = (float)args[2].asNumber();
        float a = 1.0f;
        const char *text = nullptr;
        if (argCount == 4)
        {
            text = args[3].asStringChars();
        }
        else
        {
            a = (float)args[3].asNumber();
            text = args[4].asStringChars();
        }

        if (argCount == 4)
            ImGui::TextColored(ImVec4(r, g, b, 1.0f), "%s", text);
        else
            ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
        return 0;
    }

    int TextDisabled(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.TextDisabled()"))
            return 0;

        if (argCount != 1 || !args[0].isString())
        {
            vm->runtimeError("ImGui.TextDisabled expects (text)");
            return 0;
        }

        ImGui::TextDisabled("%s", args[0].asStringChars());
        return 0;
    }

    int TextWrapped(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.TextWrapped()"))
            return 0;

        if (argCount != 1 || !args[0].isString())
        {
            vm->runtimeError("ImGui.TextWrapped expects (text)");
            return 0;
        }

        ImGui::TextWrapped("%s", args[0].asStringChars());
        return 0;
    }

    int BulletText(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.BulletText()"))
            return 0;

        if (argCount != 1 || !args[0].isString())
        {
            vm->runtimeError("ImGui.BulletText expects (text)");
            return 0;
        }

        ImGui::BulletText("%s", args[0].asStringChars());
        return 0;
    }

    int SeparatorText(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SeparatorText()"))
            return 0;

        if (argCount != 1 || !args[0].isString())
        {
            vm->runtimeError("ImGui.SeparatorText expects (text)");
            return 0;
        }

        ImGui::SeparatorText(args[0].asStringChars());
        return 0;
    }

    void register_text(ModuleBuilder &module)
    {
        module.addFunction("Text", Text, 1);
        module.addFunction("TextColored", TextColored, -1);
        module.addFunction("TextDisabled", TextDisabled, 1);
        module.addFunction("TextWrapped", TextWrapped, 1);
        module.addFunction("BulletText", BulletText, 1);
        module.addFunction("SeparatorText", SeparatorText, 1);
    }
}
