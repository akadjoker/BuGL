#pragma once
#include "interpreter.hpp"

namespace RecastBindings
{
    void registerAll(Interpreter &vm);
    void cleanup();
}
