#include "micropather_core.hpp"

#include <algorithm>
#include <cmath>

namespace MicroPatherBindings
{
    NativeClassDef *g_gridPathfinderClass = nullptr;

    int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    bool read_int_arg(const Value &value, int *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNumber())
        {
            Error("%s arg %d expects number", fn, argIndex);
            return false;
        }

        *out = (int)value.asNumber();
        return true;
    }

    bool read_boolish_arg(const Value &value, bool *out, const char *fn, int argIndex)
    {
        if (!out)
            return false;

        if (value.isBool())
        {
            *out = value.asBool();
            return true;
        }

        if (value.isNumber())
        {
            *out = value.asNumber() != 0.0;
            return true;
        }

        Error("%s arg %d expects bool or number", fn, argIndex);
        return false;
    }

    GridPathfinderHandle *require_grid(void *instance, const char *fn)
    {
        GridPathfinderHandle *h = (GridPathfinderHandle *)instance;
        if (!h || !h->valid || !h->graph || !h->solver)
        {
            Error("%s: invalid or destroyed GridPathfinder", fn);
            return nullptr;
        }
        return h;
    }

    GridGraph::GridGraph(int w, int h, bool diagonal)
    {
        width = w;
        height = h;
        allowDiagonal = diagonal;
        blocked.assign((size_t)width * (size_t)height, 0);
    }

    int GridGraph::indexOf(int x, int y) const
    {
        return y * width + x;
    }

    bool GridGraph::inBounds(int x, int y) const
    {
        return x >= 0 && y >= 0 && x < width && y < height;
    }

    bool GridGraph::isBlocked(int x, int y) const
    {
        if (!inBounds(x, y))
            return true;
        return blocked[(size_t)indexOf(x, y)] != 0;
    }

    void GridGraph::setBlocked(int x, int y, bool blockedValue)
    {
        if (!inBounds(x, y))
            return;
        blocked[(size_t)indexOf(x, y)] = blockedValue ? 1u : 0u;
    }

    void GridGraph::clear(bool blockedValue)
    {
        std::fill(blocked.begin(), blocked.end(), blockedValue ? 1u : 0u);
    }

    int GridGraph::setRectBlocked(int x, int y, int w, int h, bool blockedValue)
    {
        int changed = 0;
        for (int yy = 0; yy < h; ++yy)
        {
            for (int xx = 0; xx < w; ++xx)
            {
                int px = x + xx;
                int py = y + yy;
                if (!inBounds(px, py))
                    continue;
                blocked[(size_t)indexOf(px, py)] = blockedValue ? 1u : 0u;
                changed++;
            }
        }
        return changed;
    }

    void GridGraph::setDiagonal(bool diagonal)
    {
        allowDiagonal = diagonal;
    }

    void *GridGraph::encodeState(int x, int y) const
    {
        if (!inBounds(x, y))
            return nullptr;
        uintptr_t encoded = (uintptr_t)indexOf(x, y) + 1u;
        return (void *)encoded;
    }

    bool GridGraph::decodeState(void *state, int *x, int *y) const
    {
        if (!state || !x || !y)
            return false;
        uintptr_t raw = (uintptr_t)state;
        if (raw == 0)
            return false;
        int idx = (int)(raw - 1u);
        if (idx < 0 || idx >= width * height)
            return false;
        *x = idx % width;
        *y = idx / width;
        return true;
    }

    float GridGraph::LeastCostEstimate(void *stateStart, void *stateEnd)
    {
        int sx = 0, sy = 0, ex = 0, ey = 0;
        if (!decodeState(stateStart, &sx, &sy) || !decodeState(stateEnd, &ex, &ey))
            return 0.0f;

        int dx = std::abs(ex - sx);
        int dy = std::abs(ey - sy);
        if (!allowDiagonal)
            return (float)(dx + dy);

        int diag = std::min(dx, dy);
        int straight = std::max(dx, dy) - diag;
        return (float)diag * 1.41421356f + (float)straight;
    }

    void GridGraph::AdjacentCost(void *state, MP_VECTOR<micropather::StateCost> *adjacent)
    {
        if (!adjacent)
            return;
        adjacent->clear();

        int x = 0, y = 0;
        if (!decodeState(state, &x, &y))
            return;

        static const int dirs4[4][2] = {
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        static const int dirs8[8][2] = {
            {1, 0}, {-1, 0}, {0, 1}, {0, -1},
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

        const int (*dirs)[2] = allowDiagonal ? dirs8 : dirs4;
        int nDirs = allowDiagonal ? 8 : 4;

        for (int i = 0; i < nDirs; ++i)
        {
            int nx = x + dirs[i][0];
            int ny = y + dirs[i][1];
            if (!inBounds(nx, ny) || isBlocked(nx, ny))
                continue;

            if (allowDiagonal && i >= 4)
            {
                int ox = x + dirs[i][0];
                int oy = y;
                int px = x;
                int py = y + dirs[i][1];
                if (isBlocked(ox, oy) || isBlocked(px, py))
                    continue;
            }

            micropather::StateCost sc;
            sc.state = encodeState(nx, ny);
            sc.cost = (allowDiagonal && i >= 4) ? 1.41421356f : 1.0f;
            adjacent->push_back(sc);
        }
    }

    void GridGraph::PrintStateInfo(void *state)
    {
        int x = 0, y = 0;
        if (!decodeState(state, &x, &y))
            return;
        Print("(%d,%d)", x, y);
    }
}
