#include "bindings_internal.hpp"

namespace ImGuiBindings
{
    int SameLine(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.SameLine()"))
            return 0;

        if (argCount == 0)
        {
            ImGui::SameLine();
            return 0;
        }

        if (argCount == 2 && args[0].isNumber() && args[1].isNumber())
        {
            ImGui::SameLine((float)args[0].asNumber(), (float)args[1].asNumber());
            return 0;
        }

        vm->runtimeError("ImGui.SameLine expects () or (offsetFromStartX, spacing)");
        return 0;
    }

    int Separator(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.Separator()"))
            return 0;

        ImGui::Separator();
        return 0;
    }

    int Spacing(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.Spacing()"))
            return 0;

        ImGui::Spacing();
        return 0;
    }

    int NewLine(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.NewLine()"))
            return 0;

        ImGui::NewLine();
        return 0;
    }

    int Dummy(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Dummy()"))
            return 0;

        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            vm->runtimeError("ImGui.Dummy expects (width, height)");
            return 0;
        }

        ImGui::Dummy(ImVec2((float)args[0].asNumber(), (float)args[1].asNumber()));
        return 0;
    }

    int Indent(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Indent()"))
            return 0;

        float indent = 0.0f;
        if (!optional_number_arg(args, 0, argCount, 0.0f, &indent) || argCount > 1)
        {
            vm->runtimeError("ImGui.Indent expects () or (width)");
            return 0;
        }

        ImGui::Indent(indent);
        return 0;
    }

    int Unindent(Interpreter *vm, int argCount, Value *args)
    {
        if (!ensure_context(vm, "ImGui.Unindent()"))
            return 0;

        float indent = 0.0f;
        if (!optional_number_arg(args, 0, argCount, 0.0f, &indent) || argCount > 1)
        {
            vm->runtimeError("ImGui.Unindent expects () or (width)");
            return 0;
        }

        ImGui::Unindent(indent);
        return 0;
    }

    int BeginGroup(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.BeginGroup()"))
            return 0;

        ImGui::BeginGroup();
        return 0;
    }

    int EndGroup(Interpreter *vm, int argCount, Value *args)
    {
        (void)argCount;
        (void)args;
        if (!ensure_context(vm, "ImGui.EndGroup()"))
            return 0;

        ImGui::EndGroup();
        return 0;
    }

    void register_layout(ModuleBuilder &module)
    {
        module.addFunction("SameLine", SameLine, -1);
        module.addFunction("Separator", Separator, 0);
        module.addFunction("Spacing", Spacing, 0);
        module.addFunction("NewLine", NewLine, 0);
        module.addFunction("Dummy", Dummy, 2);
        module.addFunction("Indent", Indent, -1);
        module.addFunction("Unindent", Unindent, -1);
        module.addFunction("BeginGroup", BeginGroup, 0);
        module.addFunction("EndGroup", EndGroup, 0);
    }
}
