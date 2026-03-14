#pragma once

#include "interpreter.hpp"
#include "plugin.hpp"

namespace AssimpBindings
{
    void registerAll(Interpreter &vm);
    void cleanup();
}
