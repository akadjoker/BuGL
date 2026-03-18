#pragma once

#include "interpreter.hpp"

namespace MeshOptBindings
{
    void registerAll(Interpreter &vm);
    void cleanup();
}
