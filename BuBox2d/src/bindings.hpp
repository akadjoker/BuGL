#pragma once

#include "interpreter.hpp"


namespace BOX2DBindings
{
    void register_box2d(Interpreter &vm);
    void register_box2d_joints(Interpreter &vm);
}
