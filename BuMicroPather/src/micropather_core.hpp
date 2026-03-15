#pragma once

#include "bindings.hpp"
#include "micropather.h"

#include <cstdint>
#include <vector>

namespace MicroPatherBindings
{
    class GridGraph final : public micropather::Graph
    {
    public:
        GridGraph(int w, int h, bool diagonal);

        bool inBounds(int x, int y) const;
        bool isBlocked(int x, int y) const;
        void setBlocked(int x, int y, bool blockedValue);
        void clear(bool blockedValue);
        int setRectBlocked(int x, int y, int w, int h, bool blockedValue);
        void setDiagonal(bool diagonal);

        void *encodeState(int x, int y) const;
        bool decodeState(void *state, int *x, int *y) const;

        int width = 0;
        int height = 0;
        bool allowDiagonal = false;

        float LeastCostEstimate(void *stateStart, void *stateEnd) override;
        void AdjacentCost(void *state, MP_VECTOR<micropather::StateCost> *adjacent) override;
        void PrintStateInfo(void *state) override;

    private:
        int indexOf(int x, int y) const;
        std::vector<uint8_t> blocked;
    };

    struct GridPathfinderHandle
    {
        GridGraph *graph = nullptr;
        micropather::MicroPather *solver = nullptr;
        bool valid = false;
        int lastStatus = micropather::MicroPather::NO_SOLUTION;
        float lastCost = 0.0f;

        void destroy()
        {
            if (solver)
            {
                delete solver;
                solver = nullptr;
            }
            if (graph)
            {
                delete graph;
                graph = nullptr;
            }
            valid = false;
            lastStatus = micropather::MicroPather::NO_SOLUTION;
            lastCost = 0.0f;
        }
    };

    extern NativeClassDef *g_gridPathfinderClass;

    int push_nil1(Interpreter *vm);
    bool read_int_arg(const Value &value, int *out, const char *fn, int argIndex);
    bool read_boolish_arg(const Value &value, bool *out, const char *fn, int argIndex);
    GridPathfinderHandle *require_grid(void *instance, const char *fn);

    void register_grid(Interpreter &vm);
}
