#pragma once

#include "interpreter.hpp"

namespace JoltBindings
{
    void registerAll(Interpreter &vm);
    void cleanup();
}
