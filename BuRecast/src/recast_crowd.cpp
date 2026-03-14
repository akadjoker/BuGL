#include "recast_core.hpp"

namespace RecastBindings
{
    // ── Constructor: NavCrowd(navmesh [, maxAgents]) ─────────
    static void *crowd_ctor(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount < 1)
        {
            Error("NavCrowd(navmesh [, maxAgents])");
            return nullptr;
        }

        // First arg must be a NavMesh native class instance
        if (!args[0].isNativeClassInstance())
        {
            Error("NavCrowd: arg 1 must be a NavMesh");
            return nullptr;
        }

        NativeClassInstance *nci = args[0].asNativeClassInstance();
        if (!nci || !nci->instance)
        {
            Error("NavCrowd: invalid NavMesh argument");
            return nullptr;
        }

        NavMeshHandle *mesh = (NavMeshHandle *)nci->instance;
        if (!mesh->valid || !mesh->navMesh)
        {
            Error("NavCrowd: NavMesh is not built");
            return nullptr;
        }

        int maxAgents = 128;
        if (argCount > 1 && args[1].isNumber())
            maxAgents = (int)args[1].asDouble();
        if (maxAgents < 1) maxAgents = 1;

        dtCrowd *crowd = dtAllocCrowd();
        if (!crowd->init(maxAgents, 0.6f, mesh->navMesh))
        {
            dtFreeCrowd(crowd);
            Error("NavCrowd: crowd init failed");
            return nullptr;
        }

        NavCrowdHandle *h = new NavCrowdHandle();
        h->crowd = crowd;
        h->mesh  = mesh;
        h->valid = true;
        return h;
    }

    static void crowd_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        NavCrowdHandle *h = (NavCrowdHandle *)instance;
        if (h) { h->destroy(); delete h; }
    }

    // ── NavCrowd.addAgent(pos [, maxSpeed, maxAccel, radius, height]) → int
    static int crowd_add_agent(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1)
        {
            Error("NavCrowd.addAgent(pos [, maxSpeed, maxAccel, radius, height])");
            return push_nil1(vm);
        }

        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.addAgent()");
        if (!h) return push_nil1(vm);

        Vector3 pos;
        if (!read_vector3_arg(args[0], &pos, "NavCrowd.addAgent()", 1)) return push_nil1(vm);

        dtCrowdAgentParams ap;
        memset(&ap, 0, sizeof(ap));
        ap.radius                = argCount > 3 && args[3].isNumber() ? (float)args[3].asDouble() : 0.6f;
        ap.height                = argCount > 4 && args[4].isNumber() ? (float)args[4].asDouble() : 2.0f;
        ap.maxAcceleration       = argCount > 2 && args[2].isNumber() ? (float)args[2].asDouble() : 8.0f;
        ap.maxSpeed              = argCount > 1 && args[1].isNumber() ? (float)args[1].asDouble() : 3.5f;
        ap.collisionQueryRange   = ap.radius * 12.0f;
        ap.pathOptimizationRange = ap.radius * 30.0f;
        ap.updateFlags           = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS |
                                   DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
        ap.obstacleAvoidanceType = 3;
        ap.separationWeight      = 2.0f;

        const float p[3] = {pos.x, pos.y, pos.z};
        int id = h->crowd->addAgent(p, &ap);
        vm->push(vm->makeInt(id));
        return 1;
    }

    // ── NavCrowd.removeAgent(id) ─────────────────────────────
    static int crowd_remove_agent(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1) { Error("NavCrowd.removeAgent(id)"); return 0; }
        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.removeAgent()");
        if (!h) return 0;
        h->crowd->removeAgent((int)args[0].asDouble());
        return 0;
    }

    // ── NavCrowd.setTarget(id, pos) → bool ──────────────────
    static int crowd_set_target(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2) { Error("NavCrowd.setTarget(id, pos)"); return push_nil1(vm); }
        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.setTarget()");
        if (!h) return push_nil1(vm);

        int id = (int)args[0].asDouble();
        Vector3 target;
        if (!read_vector3_arg(args[1], &target, "NavCrowd.setTarget()", 2)) return push_nil1(vm);

        dtQueryFilter filter;
        filter.setIncludeFlags(0xffff);
        filter.setExcludeFlags(0);

        float ext[3] = {2.0f, 4.0f, 2.0f};
        dtPolyRef ref = 0;
        float nearestPt[3];
        const float tp[3] = {target.x, target.y, target.z};
        h->mesh->navQuery->findNearestPoly(tp, ext, &filter, &ref, nearestPt);

        bool ok = false;
        if (ref)
            ok = h->crowd->requestMoveTarget(id, ref, nearestPt);

        vm->pushBool(ok);
        return 1;
    }

    // ── NavCrowd.update(dt) ───────────────────────────────────
    static int crowd_update(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1) { Error("NavCrowd.update(dt)"); return 0; }
        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.update()");
        if (!h) return 0;
        h->crowd->update((float)args[0].asDouble(), nullptr);
        return 0;
    }

    // ── NavCrowd.getPosition(id) → Vector3 | nil ────────────
    static int crowd_get_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("NavCrowd.getPosition(id)"); return push_nil1(vm); }
        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.getPosition()");
        if (!h) return push_nil1(vm);

        const dtCrowdAgent *ag = h->crowd->getAgent((int)args[0].asDouble());
        if (!ag || !ag->active) return push_nil1(vm);

        Vector3 pos = {ag->npos[0], ag->npos[1], ag->npos[2]};
        return push_vector3(vm, pos) ? 1 : push_nil1(vm);
    }

    // ── NavCrowd.getVelocity(id) → Vector3 | nil ────────────
    static int crowd_get_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("NavCrowd.getVelocity(id)"); return push_nil1(vm); }
        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.getVelocity()");
        if (!h) return push_nil1(vm);

        const dtCrowdAgent *ag = h->crowd->getAgent((int)args[0].asDouble());
        if (!ag || !ag->active) return push_nil1(vm);

        Vector3 vel = {ag->vel[0], ag->vel[1], ag->vel[2]};
        return push_vector3(vm, vel) ? 1 : push_nil1(vm);
    }

    // ── NavCrowd.isAgentActive(id) → bool ────────────────────
    static int crowd_is_active(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("NavCrowd.isAgentActive(id)"); return push_nil1(vm); }
        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.isAgentActive()");
        if (!h) { vm->pushBool(false); return 1; }
        const dtCrowdAgent *ag = h->crowd->getAgent((int)args[0].asDouble());
        vm->pushBool(ag && ag->active);
        return 1;
    }

    // ── NavCrowd.getAgentCount() → int ──────────────────────
    static int crowd_agent_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0) { Error("NavCrowd.getAgentCount() expects 0 arguments"); return push_nil1(vm); }
        NavCrowdHandle *h = require_crowd(instance, "NavCrowd.getAgentCount()");
        if (!h) { vm->push(vm->makeInt(0)); return 1; }
        vm->push(vm->makeInt(h->crowd->getAgentCount()));
        return 1;
    }

    // ── NavCrowd.destroy() ───────────────────────────────────
    static int crowd_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm; (void)args;
        if (argCount != 0) { Error("NavCrowd.destroy() expects 0 arguments"); return 0; }
        NavCrowdHandle *h = (NavCrowdHandle *)instance;
        if (h) h->destroy();
        return 0;
    }

    // ── Registration ─────────────────────────────────────────
    void register_crowd(Interpreter &vm)
    {
        g_navCrowdClass = vm.addNativeClass("NavCrowd",
            crowd_ctor, crowd_dtor, sizeof(void *));

        vm.addNativeMethod(g_navCrowdClass, "addAgent",       crowd_add_agent);
        vm.addNativeMethod(g_navCrowdClass, "removeAgent",    crowd_remove_agent);
        vm.addNativeMethod(g_navCrowdClass, "setTarget",      crowd_set_target);
        vm.addNativeMethod(g_navCrowdClass, "update",         crowd_update);
        vm.addNativeMethod(g_navCrowdClass, "getPosition",    crowd_get_position);
        vm.addNativeMethod(g_navCrowdClass, "getVelocity",    crowd_get_velocity);
        vm.addNativeMethod(g_navCrowdClass, "isAgentActive",  crowd_is_active);
        vm.addNativeMethod(g_navCrowdClass, "getAgentCount",  crowd_agent_count);
        vm.addNativeMethod(g_navCrowdClass, "destroy",        crowd_destroy);
    }

} // namespace RecastBindings
