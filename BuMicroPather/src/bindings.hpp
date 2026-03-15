#pragma once

#include "interpreter.hpp"

namespace MicroPatherBindings
{
    void registerAll(Interpreter &vm);
    void cleanup();
}
