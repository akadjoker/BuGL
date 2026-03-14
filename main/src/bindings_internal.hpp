#pragma once

#include "bindings.hpp"
#include "imgui.h"

namespace ImGuiBindings
{
    bool sync_context(Interpreter *vm, const char *fn);
    bool ensure_context(Interpreter *vm, const char *fn);
    int push_nil(Interpreter *vm);
    int push_nils(Interpreter *vm, int count);
    bool optional_number_arg(Value *args, int index, int argCount, float defaultValue, float *out);
    bool optional_int_arg(Value *args, int index, int argCount, int defaultValue, int *out);
    bool optional_string_arg(Value *args, int index, int argCount, const char *defaultValue, const char **out);

    void register_core(ModuleBuilder &module);
    void register_layout(ModuleBuilder &module);
    void register_style(ModuleBuilder &module);
    void register_text(ModuleBuilder &module);
    void register_inputs(ModuleBuilder &module);
}
