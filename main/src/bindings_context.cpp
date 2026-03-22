#include "bindings_internal.hpp"

namespace ImGuiBindings
{
    bool sync_context(Interpreter *vm, const char *fn)
    {
        Value ctxVal;
        if (!vm->tryGetGlobal("__imgui_context", &ctxVal) || !ctxVal.isPointer() || ctxVal.asPointer() == nullptr)
        {
            vm->runtimeError("%s requires SDL.InitImGui()", fn);
            return false;
        }

        ImGuiContext *ctx = static_cast<ImGuiContext *>(ctxVal.asPointer());
        if (ImGui::GetCurrentContext() != ctx)
            ImGui::SetCurrentContext(ctx);

        return true;
    }

    bool ensure_context(Interpreter *vm, const char *fn)
    {
        return sync_context(vm, fn);
    }

    int push_nil(Interpreter *vm)
    {
        vm->push(vm->makeNil());
        return 1;
    }

     int push_false(Interpreter *vm)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }
    int push_nils(Interpreter *vm, int count)
    {
        for (int i = 0; i < count; ++i)
            vm->push(vm->makeNil());
        return count;
    }

    int push_falses(Interpreter *vm, int count)
    {
        for (int i = 0; i < count; ++i)
            vm->push(vm->makeBool(false));
        return count;
    }

    bool optional_number_arg(Value *args, int index, int argCount, float defaultValue, float *out)
    {
        if (index >= argCount)
        {
            *out = defaultValue;
            return true;
        }
        if (!args[index].isNumber())
            return false;
        *out = (float)args[index].asNumber();
        return true;
    }

    bool optional_int_arg(Value *args, int index, int argCount, int defaultValue, int *out)
    {
        if (index >= argCount)
        {
            *out = defaultValue;
            return true;
        }
        if (!args[index].isNumber())
            return false;
        *out = (int)args[index].asNumber();
        return true;
    }

    bool optional_string_arg(Value *args, int index, int argCount, const char *defaultValue, const char **out)
    {
        if (index >= argCount)
        {
            *out = defaultValue;
            return true;
        }
        if (!args[index].isString())
            return false;
        *out = args[index].asStringChars();
        return true;
    }
}
