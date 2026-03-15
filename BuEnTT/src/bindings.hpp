#pragma once

#include "interpreter.hpp"

namespace EnTTBindings
{
    void registerAll(Interpreter &vm);
    void cleanup();
}
