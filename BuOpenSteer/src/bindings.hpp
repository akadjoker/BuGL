#pragma once

#include "interpreter.hpp"

namespace OpenSteerBindings
{
    void registerAll(Interpreter &vm);
    void cleanup();
}
