#include "bindings.hpp"
#include <ode/ode.h>

namespace ODEBindings
{
    static int g_ode_init_done = 0;

    static int ensure_ode_init()
    {
        if (g_ode_init_done)
            return 1;
        if (dInitODE2(0) == 0)
            return 0;
        g_ode_init_done = 1;
        return 1;
    }

    static int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    static int push_nil3(Interpreter *vm)
    {
        vm->pushNil();
        vm->pushNil();
        vm->pushNil();
        return 3;
    }

    static int push_nil11(Interpreter *vm)
    {
        for (int i = 0; i < 11; ++i)
            vm->pushNil();
        return 11;
    }

    static int push_nil12(Interpreter *vm)
    {
        for (int i = 0; i < 12; ++i)
            vm->pushNil();
        return 12;
    }

    static int read_pointer_arg(Value *args, int index, void **out, const char *fnName, const char *typeName)
    {
        if (!args[index].isPointer())
        {
            Error("%s arg %d expects %s pointer", fnName, index + 1, typeName);
            return 0;
        }
        *out = args[index].asPointer();
        if (!*out)
        {
            Error("%s arg %d got null %s pointer", fnName, index + 1, typeName);
            return 0;
        }
        return 1;
    }

    static int read_space_or_nil_arg(Value *args, int index, dSpaceID *out, const char *fnName)
    {
        if (args[index].isNil())
        {
            *out = (dSpaceID)0;
            return 1;
        }
        if (!args[index].isPointer())
        {
            Error("%s arg %d expects space pointer or nil", fnName, index + 1);
            return 0;
        }
        *out = (dSpaceID)args[index].asPointer();
        return 1;
    }

    static int read_body_or_nil_arg(Value *args, int index, dBodyID *out, const char *fnName)
    {
        if (args[index].isNil())
        {
            *out = (dBodyID)0;
            return 1;
        }
        if (!args[index].isPointer())
        {
            Error("%s arg %d expects body pointer or nil", fnName, index + 1);
            return 0;
        }
        *out = (dBodyID)args[index].asPointer();
        return 1;
    }

    static int read_joint_group_or_nil_arg(Value *args, int index, dJointGroupID *out, const char *fnName)
    {
        if (args[index].isNil())
        {
            *out = (dJointGroupID)0;
            return 1;
        }
        if (!args[index].isPointer())
        {
            Error("%s arg %d expects joint group pointer or nil", fnName, index + 1);
            return 0;
        }
        *out = (dJointGroupID)args[index].asPointer();
        return 1;
    }

    static int push_dvec3(Interpreter *vm, const dVector3 v)
    {
        vm->pushDouble((double)v[0]);
        vm->pushDouble((double)v[1]);
        vm->pushDouble((double)v[2]);
        return 3;
    }

    // ---------------- Init ----------------

    static int native_dInitODE2(Interpreter *vm, int argc, Value *args)
    {
        unsigned int flags = 0;
        if (argc > 1)
        {
            Error("dInitODE2 expects 0 or 1 arguments");
            return push_nil1(vm);
        }
        if (argc == 1)
            flags = (unsigned int)args[0].asInt();
        int ok = dInitODE2(flags);
        if (ok)
            g_ode_init_done = 1;
        vm->pushBool(ok != 0);
        return 1;
    }

    static int native_dCloseODE(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("dCloseODE expects 0 arguments");
            return push_nil1(vm);
        }
        if (g_ode_init_done)
        {
            dCloseODE();
            g_ode_init_done = 0;
        }
        vm->pushBool(true);
        return 1;
    }

    // ---------------- World ----------------

    static int native_dWorldCreate(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("dWorldCreate expects 0 arguments");
            return push_nil1(vm);
        }
        if (!ensure_ode_init())
        {
            Error("dWorldCreate failed: ODE init failed");
            return push_nil1(vm);
        }
        dWorldID w = dWorldCreate();
        vm->pushPointer((void *)w);
        return 1;
    }

    static int native_dWorldDestroy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dWorldDestroy expects 1 argument");
            return 0;
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldDestroy", "world"))
            return 0;
        dWorldDestroy(w);
        return 0;
    }

    static int native_dWorldSetGravity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dWorldSetGravity expects (world, x, y, z)");
            return 0;
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldSetGravity", "world"))
            return 0;
        dWorldSetGravity(w, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dWorldGetGravity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dWorldGetGravity expects (world)");
            return push_nil3(vm);
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldGetGravity", "world"))
            return push_nil3(vm);

        dVector3 g = {0, 0, 0, 0};
        dWorldGetGravity(w, g);
        vm->pushDouble((double)g[0]);
        vm->pushDouble((double)g[1]);
        vm->pushDouble((double)g[2]);
        return 3;
    }

    static int native_dWorldSetERP(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dWorldSetERP expects (world, erp)");
            return 0;
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldSetERP", "world"))
            return 0;
        dWorldSetERP(w, (dReal)args[1].asNumber());
        return 0;
    }

    static int native_dWorldGetERP(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dWorldGetERP expects (world)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldGetERP", "world"))
            return push_nil1(vm);
        vm->pushDouble((double)dWorldGetERP(w));
        return 1;
    }

    static int native_dWorldSetCFM(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dWorldSetCFM expects (world, cfm)");
            return 0;
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldSetCFM", "world"))
            return 0;
        dWorldSetCFM(w, (dReal)args[1].asNumber());
        return 0;
    }

    static int native_dWorldGetCFM(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dWorldGetCFM expects (world)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldGetCFM", "world"))
            return push_nil1(vm);
        vm->pushDouble((double)dWorldGetCFM(w));
        return 1;
    }

    static int native_dWorldSetQuickStepNumIterations(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dWorldSetQuickStepNumIterations expects (world, iterations)");
            return 0;
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldSetQuickStepNumIterations", "world"))
            return 0;
        dWorldSetQuickStepNumIterations(w, args[1].asInt());
        return 0;
    }

    static int native_dWorldGetQuickStepNumIterations(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dWorldGetQuickStepNumIterations expects (world)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldGetQuickStepNumIterations", "world"))
            return push_nil1(vm);
        vm->pushInt(dWorldGetQuickStepNumIterations(w));
        return 1;
    }

    static int native_dWorldStep(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dWorldStep expects (world, dt)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldStep", "world"))
            return push_nil1(vm);
        int ok = dWorldStep(w, (dReal)args[1].asNumber());
        vm->pushInt(ok);
        return 1;
    }

    static int native_dWorldQuickStep(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dWorldQuickStep expects (world, dt)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dWorldQuickStep", "world"))
            return push_nil1(vm);
        int ok = dWorldQuickStep(w, (dReal)args[1].asNumber());
        vm->pushInt(ok);
        return 1;
    }

    // ---------------- Space ----------------

    static int native_dSimpleSpaceCreate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dSimpleSpaceCreate expects (parentSpaceOrNil)");
            return push_nil1(vm);
        }
        if (!ensure_ode_init())
        {
            Error("dSimpleSpaceCreate failed: ODE init failed");
            return push_nil1(vm);
        }
        dSpaceID parent = 0;
        if (!read_space_or_nil_arg(args, 0, &parent, "dSimpleSpaceCreate"))
            return push_nil1(vm);
        dSpaceID s = dSimpleSpaceCreate(parent);
        vm->pushPointer((void *)s);
        return 1;
    }

    static int native_dHashSpaceCreate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dHashSpaceCreate expects (parentSpaceOrNil)");
            return push_nil1(vm);
        }
        if (!ensure_ode_init())
        {
            Error("dHashSpaceCreate failed: ODE init failed");
            return push_nil1(vm);
        }
        dSpaceID parent = 0;
        if (!read_space_or_nil_arg(args, 0, &parent, "dHashSpaceCreate"))
            return push_nil1(vm);
        dSpaceID s = dHashSpaceCreate(parent);
        vm->pushPointer((void *)s);
        return 1;
    }

    static int native_dSpaceDestroy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dSpaceDestroy expects (space)");
            return 0;
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dSpaceDestroy", "space"))
            return 0;
        dSpaceDestroy(s);
        return 0;
    }

    static int native_dSpaceSetSublevel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dSpaceSetSublevel expects (space, sublevel)");
            return 0;
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dSpaceSetSublevel", "space"))
            return 0;
        dSpaceSetSublevel(s, args[1].asInt());
        return 0;
    }

    static int native_dSpaceGetSublevel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dSpaceGetSublevel expects (space)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dSpaceGetSublevel", "space"))
            return push_nil1(vm);
        vm->pushInt(dSpaceGetSublevel(s));
        return 1;
    }

    static int native_dSpaceSetCleanup(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dSpaceSetCleanup expects (space, mode)");
            return 0;
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dSpaceSetCleanup", "space"))
            return 0;
        dSpaceSetCleanup(s, args[1].asInt());
        return 0;
    }

    static int native_dSpaceGetCleanup(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dSpaceGetCleanup expects (space)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dSpaceGetCleanup", "space"))
            return push_nil1(vm);
        vm->pushInt(dSpaceGetCleanup(s));
        return 1;
    }

    static int native_dSpaceAdd(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dSpaceAdd expects (space, geom)");
            return 0;
        }
        dSpaceID s = nullptr;
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dSpaceAdd", "space"))
            return 0;
        if (!read_pointer_arg(args, 1, (void **)&g, "dSpaceAdd", "geom"))
            return 0;
        dSpaceAdd(s, g);
        return 0;
    }

    static int native_dSpaceRemove(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dSpaceRemove expects (space, geom)");
            return 0;
        }
        dSpaceID s = nullptr;
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dSpaceRemove", "space"))
            return 0;
        if (!read_pointer_arg(args, 1, (void **)&g, "dSpaceRemove", "geom"))
            return 0;
        dSpaceRemove(s, g);
        return 0;
    }

    struct ODESimpleCollideContext
    {
        dWorldID world;
        dJointGroupID group;
        int maxContacts;
        dReal mu;
        dReal bounce;
        dReal bounceVel;
        dReal softERP;
        dReal softCFM;
        int contactsCreated;
    };

    enum
    {
        kODEContactEventsMax = 2048
    };

    struct ODEContactEvent
    {
        dGeomID geom1;
        dGeomID geom2;
        dBodyID body1;
        dBodyID body2;
        dReal px;
        dReal py;
        dReal pz;
        dReal nx;
        dReal ny;
        dReal nz;
        dReal depth;
    };

    static ODEContactEvent g_ode_contact_events[kODEContactEventsMax];
    static int g_ode_contact_event_count = 0;

    static void ode_contact_events_clear()
    {
        g_ode_contact_event_count = 0;
    }

    static void ode_contact_events_push(const dContactGeom *cg)
    {
        if (!cg || g_ode_contact_event_count >= kODEContactEventsMax)
            return;

        ODEContactEvent *evt = &g_ode_contact_events[g_ode_contact_event_count++];
        evt->geom1 = cg->g1;
        evt->geom2 = cg->g2;
        evt->body1 = cg->g1 ? dGeomGetBody(cg->g1) : (dBodyID)0;
        evt->body2 = cg->g2 ? dGeomGetBody(cg->g2) : (dBodyID)0;
        evt->px = cg->pos[0];
        evt->py = cg->pos[1];
        evt->pz = cg->pos[2];
        evt->nx = cg->normal[0];
        evt->ny = cg->normal[1];
        evt->nz = cg->normal[2];
        evt->depth = cg->depth;
    }

    static void ode_near_callback_simple(void *data, dGeomID o1, dGeomID o2)
    {
        ODESimpleCollideContext *ctx = (ODESimpleCollideContext *)data;
        if (!ctx || !o1 || !o2)
            return;

        if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
        {
            dSpaceCollide2(o1, o2, data, &ode_near_callback_simple);
            if (dGeomIsSpace(o1))
                dSpaceCollide((dSpaceID)o1, data, &ode_near_callback_simple);
            if (dGeomIsSpace(o2))
                dSpaceCollide((dSpaceID)o2, data, &ode_near_callback_simple);
            return;
        }

        dBodyID b1 = dGeomGetBody(o1);
        dBodyID b2 = dGeomGetBody(o2);
        if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
            return;

        dContact contacts[16];
        int n = dCollide(o1, o2, ctx->maxContacts, &contacts[0].geom, sizeof(dContact));
        for (int i = 0; i < n; i++)
        {
            ode_contact_events_push(&contacts[i].geom);

            contacts[i].surface.mode = dContactBounce | dContactSoftERP | dContactSoftCFM | dContactApprox1;
            contacts[i].surface.mu = ctx->mu;
            contacts[i].surface.mu2 = 0;
            contacts[i].surface.bounce = ctx->bounce;
            contacts[i].surface.bounce_vel = ctx->bounceVel;
            contacts[i].surface.soft_erp = ctx->softERP;
            contacts[i].surface.soft_cfm = ctx->softCFM;
            contacts[i].surface.motion1 = 0;
            contacts[i].surface.motion2 = 0;
            contacts[i].surface.motionN = 0;
            contacts[i].surface.slip1 = 0;
            contacts[i].surface.slip2 = 0;

            dJointID j = dJointCreateContact(ctx->world, ctx->group, &contacts[i]);
            if (j)
            {
                dBodyID c1 = dGeomGetBody(contacts[i].geom.g1);
                dBodyID c2 = dGeomGetBody(contacts[i].geom.g2);
                dJointAttach(j, c1, c2);
                ctx->contactsCreated++;
            }
        }
    }

    static int native_dSpaceCollideSimple(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 3 || argc > 9)
        {
            Error("dSpaceCollideSimple expects (space, world, contactGroup[, maxContacts, mu, bounce, bounceVel, softERP, softCFM])");
            return push_nil1(vm);
        }

        dSpaceID space = nullptr;
        dWorldID world = nullptr;
        dJointGroupID group = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&space, "dSpaceCollideSimple", "space") ||
            !read_pointer_arg(args, 1, (void **)&world, "dSpaceCollideSimple", "world") ||
            !read_pointer_arg(args, 2, (void **)&group, "dSpaceCollideSimple", "joint group"))
            return push_nil1(vm);

        ODESimpleCollideContext ctx;
        ctx.world = world;
        ctx.group = group;
        ctx.maxContacts = (argc >= 4) ? args[3].asInt() : 8;
        if (ctx.maxContacts < 1)
            ctx.maxContacts = 1;
        if (ctx.maxContacts > 16)
            ctx.maxContacts = 16;

        ctx.mu = (argc >= 5) ? (dReal)args[4].asNumber() : dInfinity;
        ctx.bounce = (argc >= 6) ? (dReal)args[5].asNumber() : (dReal)0.02;
        ctx.bounceVel = (argc >= 7) ? (dReal)args[6].asNumber() : (dReal)0.10;
        ctx.softERP = (argc >= 8) ? (dReal)args[7].asNumber() : (dReal)0.80;
        ctx.softCFM = (argc >= 9) ? (dReal)args[8].asNumber() : (dReal)1e-5;
        ctx.contactsCreated = 0;

        // One collision pass = one readable contact-event batch.
        ode_contact_events_clear();
        dSpaceCollide(space, &ctx, &ode_near_callback_simple);
        vm->pushInt(ctx.contactsCreated);
        return 1;
    }

    static int native_dContactEventsClear(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("dContactEventsClear expects 0 arguments");
            return 0;
        }
        ode_contact_events_clear();
        return 0;
    }

    static int native_dContactEventsCount(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("dContactEventsCount expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushInt(g_ode_contact_event_count);
        return 1;
    }

    static int native_dContactEventGet(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dContactEventGet expects (index)");
            return push_nil11(vm);
        }
        if (!args[0].isNumber())
        {
            Error("dContactEventGet arg 1 expects number");
            return push_nil11(vm);
        }

        int index = args[0].asInt();
        if (index < 0 || index >= g_ode_contact_event_count)
            return push_nil11(vm);

        const ODEContactEvent *evt = &g_ode_contact_events[index];
        vm->pushPointer((void *)evt->geom1);
        vm->pushPointer((void *)evt->geom2);
        vm->pushPointer((void *)evt->body1);
        vm->pushPointer((void *)evt->body2);
        vm->pushDouble((double)evt->px);
        vm->pushDouble((double)evt->py);
        vm->pushDouble((double)evt->pz);
        vm->pushDouble((double)evt->nx);
        vm->pushDouble((double)evt->ny);
        vm->pushDouble((double)evt->nz);
        vm->pushDouble((double)evt->depth);
        return 11;
    }

    // ---------------- Joint Group ----------------

    static int native_dJointGroupCreate(Interpreter *vm, int argc, Value *args)
    {
        int maxSize = 0;
        if (argc > 1)
        {
            Error("dJointGroupCreate expects 0 or 1 arguments");
            return push_nil1(vm);
        }
        if (argc == 1)
            maxSize = args[0].asInt();
        if (!ensure_ode_init())
        {
            Error("dJointGroupCreate failed: ODE init failed");
            return push_nil1(vm);
        }
        dJointGroupID g = dJointGroupCreate(maxSize);
        vm->pushPointer((void *)g);
        return 1;
    }

    static int native_dJointGroupDestroy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGroupDestroy expects (group)");
            return 0;
        }
        dJointGroupID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dJointGroupDestroy", "joint group"))
            return 0;
        dJointGroupDestroy(g);
        return 0;
    }

    static int native_dJointGroupEmpty(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGroupEmpty expects (group)");
            return 0;
        }
        dJointGroupID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dJointGroupEmpty", "joint group"))
            return 0;
        dJointGroupEmpty(g);
        return 0;
    }

    // ---------------- Joint ----------------

    static int native_dJointCreateBall(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateBall expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateBall", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateBall"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateBall(w, g));
        return 1;
    }

    static int native_dJointCreateHinge(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateHinge expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateHinge", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateHinge"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateHinge(w, g));
        return 1;
    }

    static int native_dJointCreateSlider(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateSlider expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateSlider", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateSlider"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateSlider(w, g));
        return 1;
    }

    static int native_dJointCreateHinge2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateHinge2 expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateHinge2", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateHinge2"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateHinge2(w, g));
        return 1;
    }

    static int native_dJointCreateUniversal(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateUniversal expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateUniversal", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateUniversal"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateUniversal(w, g));
        return 1;
    }

    static int native_dJointCreateFixed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateFixed expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateFixed", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateFixed"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateFixed(w, g));
        return 1;
    }

    static int native_dJointCreateNull(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateNull expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateNull", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateNull"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateNull(w, g));
        return 1;
    }

    static int native_dJointCreateAMotor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateAMotor expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateAMotor", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateAMotor"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateAMotor(w, g));
        return 1;
    }

    static int native_dJointCreateLMotor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreateLMotor expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreateLMotor", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreateLMotor"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreateLMotor(w, g));
        return 1;
    }

    static int native_dJointCreatePR(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreatePR expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreatePR", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreatePR"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreatePR(w, g));
        return 1;
    }

    static int native_dJointCreatePU(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreatePU expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreatePU", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreatePU"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreatePU(w, g));
        return 1;
    }

    static int native_dJointCreatePiston(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreatePiston expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreatePiston", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreatePiston"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreatePiston(w, g));
        return 1;
    }

    static int native_dJointCreatePlane2D(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointCreatePlane2D expects (world, groupOrNil)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        dJointGroupID g = 0;
        if (!read_pointer_arg(args, 0, (void **)&w, "dJointCreatePlane2D", "world") ||
            !read_joint_group_or_nil_arg(args, 1, &g, "dJointCreatePlane2D"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointCreatePlane2D(w, g));
        return 1;
    }

    static int native_dJointDestroy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointDestroy expects (joint)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointDestroy", "joint"))
            return 0;
        dJointDestroy(j);
        return 0;
    }

    static int native_dJointAttach(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointAttach expects (joint, body1OrNil, body2OrNil)");
            return 0;
        }
        dJointID j = nullptr;
        dBodyID b1 = 0;
        dBodyID b2 = 0;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointAttach", "joint") ||
            !read_body_or_nil_arg(args, 1, &b1, "dJointAttach") ||
            !read_body_or_nil_arg(args, 2, &b2, "dJointAttach"))
            return 0;
        dJointAttach(j, b1, b2);
        return 0;
    }

    static int native_dJointEnable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointEnable expects (joint)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointEnable", "joint"))
            return 0;
        dJointEnable(j);
        return 0;
    }

    static int native_dJointDisable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointDisable expects (joint)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointDisable", "joint"))
            return 0;
        dJointDisable(j);
        return 0;
    }

    static int native_dJointIsEnabled(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointIsEnabled expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointIsEnabled", "joint"))
            return push_nil1(vm);
        vm->pushInt(dJointIsEnabled(j));
        return 1;
    }

    static int native_dJointGetType(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetType expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetType", "joint"))
            return push_nil1(vm);
        vm->pushInt((int)dJointGetType(j));
        return 1;
    }

    static int native_dJointGetBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetBody expects (joint, index)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetBody", "joint"))
            return push_nil1(vm);
        vm->pushPointer((void *)dJointGetBody(j, args[1].asInt()));
        return 1;
    }

    static int native_dAreConnected(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dAreConnected expects (body1, body2)");
            return push_nil1(vm);
        }
        dBodyID b1 = nullptr;
        dBodyID b2 = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b1, "dAreConnected", "body") ||
            !read_pointer_arg(args, 1, (void **)&b2, "dAreConnected", "body"))
            return push_nil1(vm);
        vm->pushBool(dAreConnected(b1, b2) != 0);
        return 1;
    }

    static int native_dAreConnectedExcluding(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dAreConnectedExcluding expects (body1, body2, jointType)");
            return push_nil1(vm);
        }
        dBodyID b1 = nullptr;
        dBodyID b2 = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b1, "dAreConnectedExcluding", "body") ||
            !read_pointer_arg(args, 1, (void **)&b2, "dAreConnectedExcluding", "body"))
            return push_nil1(vm);
        vm->pushBool(dAreConnectedExcluding(b1, b2, args[2].asInt()) != 0);
        return 1;
    }

    static int native_dJointSetBallAnchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetBallAnchor expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetBallAnchor", "joint"))
            return 0;
        dJointSetBallAnchor(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointGetBallAnchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetBallAnchor expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetBallAnchor", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetBallAnchor(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetBallAnchor2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetBallAnchor2 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetBallAnchor2", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetBallAnchor2(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointSetBallParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetBallParam expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetBallParam", "joint"))
            return 0;
        dJointSetBallParam(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetBallParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetBallParam expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetBallParam", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetBallParam(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointSetHingeAnchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetHingeAnchor expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetHingeAnchor", "joint"))
            return 0;
        dJointSetHingeAnchor(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointSetHingeAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetHingeAxis expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetHingeAxis", "joint"))
            return 0;
        dJointSetHingeAxis(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointGetHingeAngle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHingeAngle expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHingeAngle", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHingeAngle(j));
        return 1;
    }

    static int native_dJointGetHingeAnchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHingeAnchor expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHingeAnchor", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetHingeAnchor(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetHingeAnchor2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHingeAnchor2 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHingeAnchor2", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetHingeAnchor2(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetHingeAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHingeAxis expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHingeAxis", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetHingeAxis(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetHingeAngleRate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHingeAngleRate expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHingeAngleRate", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHingeAngleRate(j));
        return 1;
    }

    static int native_dJointSetHingeParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetHingeParam expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetHingeParam", "joint"))
            return 0;
        dJointSetHingeParam(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetHingeParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetHingeParam expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHingeParam", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHingeParam(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointSetSliderAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetSliderAxis expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetSliderAxis", "joint"))
            return 0;
        dJointSetSliderAxis(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointGetSliderPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetSliderPosition expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetSliderPosition", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetSliderPosition(j));
        return 1;
    }

    static int native_dJointGetSliderAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetSliderAxis expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetSliderAxis", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetSliderAxis(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetSliderPositionRate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetSliderPositionRate expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetSliderPositionRate", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetSliderPositionRate(j));
        return 1;
    }

    static int native_dJointSetSliderParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetSliderParam expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetSliderParam", "joint"))
            return 0;
        dJointSetSliderParam(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetSliderParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetSliderParam expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetSliderParam", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetSliderParam(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointSetHinge2Anchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetHinge2Anchor expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetHinge2Anchor", "joint"))
            return 0;
        dJointSetHinge2Anchor(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointSetHinge2Axis1(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetHinge2Axis1 expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetHinge2Axis1", "joint"))
            return 0;
        dVector3 axis1 = {(dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber(), 0};
        dVector3 axis2 = {0, 0, 0, 0};
        dJointGetHinge2Axis2(j, axis2);
        dJointSetHinge2Axes(j, axis1, axis2);
        return 0;
    }

    static int native_dJointSetHinge2Axis2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetHinge2Axis2 expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetHinge2Axis2", "joint"))
            return 0;
        dVector3 axis1 = {0, 0, 0, 0};
        dVector3 axis2 = {(dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber(), 0};
        dJointGetHinge2Axis1(j, axis1);
        dJointSetHinge2Axes(j, axis1, axis2);
        return 0;
    }

    static int native_dJointSetHinge2Param(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetHinge2Param expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetHinge2Param", "joint"))
            return 0;
        dJointSetHinge2Param(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetHinge2Anchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Anchor expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Anchor", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetHinge2Anchor(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetHinge2Anchor2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Anchor2 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Anchor2", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetHinge2Anchor2(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetHinge2Axis1(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Axis1 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Axis1", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetHinge2Axis1(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetHinge2Axis2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Axis2 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Axis2", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetHinge2Axis2(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetHinge2Param(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetHinge2Param expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Param", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHinge2Param(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointGetHinge2Angle1(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Angle1 expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Angle1", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHinge2Angle1(j));
        return 1;
    }

    static int native_dJointGetHinge2Angle2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Angle2 expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Angle2", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHinge2Angle2(j));
        return 1;
    }

    static int native_dJointGetHinge2Angle1Rate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Angle1Rate expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Angle1Rate", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHinge2Angle1Rate(j));
        return 1;
    }

    static int native_dJointGetHinge2Angle2Rate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetHinge2Angle2Rate expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetHinge2Angle2Rate", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetHinge2Angle2Rate(j));
        return 1;
    }

    static int native_dJointSetUniversalAnchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetUniversalAnchor expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetUniversalAnchor", "joint"))
            return 0;
        dJointSetUniversalAnchor(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointSetUniversalAxis1(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetUniversalAxis1 expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetUniversalAxis1", "joint"))
            return 0;
        dJointSetUniversalAxis1(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointSetUniversalAxis2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointSetUniversalAxis2 expects (joint, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetUniversalAxis2", "joint"))
            return 0;
        dJointSetUniversalAxis2(j, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointSetUniversalParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetUniversalParam expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetUniversalParam", "joint"))
            return 0;
        dJointSetUniversalParam(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetUniversalAnchor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAnchor expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAnchor", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetUniversalAnchor(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetUniversalAnchor2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAnchor2 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAnchor2", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetUniversalAnchor2(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetUniversalAxis1(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAxis1 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAxis1", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetUniversalAxis1(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetUniversalAxis2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAxis2 expects (joint)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAxis2", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetUniversalAxis2(j, v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetUniversalParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetUniversalParam expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalParam", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetUniversalParam(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointGetUniversalAngle1(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAngle1 expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAngle1", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetUniversalAngle1(j));
        return 1;
    }

    static int native_dJointGetUniversalAngle2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAngle2 expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAngle2", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetUniversalAngle2(j));
        return 1;
    }

    static int native_dJointGetUniversalAngle1Rate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAngle1Rate expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAngle1Rate", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetUniversalAngle1Rate(j));
        return 1;
    }

    static int native_dJointGetUniversalAngle2Rate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAngle2Rate expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAngle2Rate", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetUniversalAngle2Rate(j));
        return 1;
    }

    static int native_dJointGetUniversalAngles(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetUniversalAngles expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetUniversalAngles", "joint"))
            return push_nil1(vm);
        dReal a1 = 0.0;
        dReal a2 = 0.0;
        dJointGetUniversalAngles(j, &a1, &a2);
        vm->pushDouble((double)a1);
        vm->pushDouble((double)a2);
        return 2;
    }

    static int native_dJointSetFixed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointSetFixed expects (joint)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetFixed", "joint"))
            return 0;
        dJointSetFixed(j);
        return 0;
    }

    static int native_dJointSetFixedParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetFixedParam expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetFixedParam", "joint"))
            return 0;
        dJointSetFixedParam(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetFixedParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetFixedParam expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetFixedParam", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetFixedParam(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointSetAMotorMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointSetAMotorMode expects (joint, mode)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetAMotorMode", "joint"))
            return 0;
        dJointSetAMotorMode(j, args[1].asInt());
        return 0;
    }

    static int native_dJointSetAMotorNumAxes(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointSetAMotorNumAxes expects (joint, numAxes)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetAMotorNumAxes", "joint"))
            return 0;
        dJointSetAMotorNumAxes(j, args[1].asInt());
        return 0;
    }

    static int native_dJointSetAMotorAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("dJointSetAMotorAxis expects (joint, anum, rel, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetAMotorAxis", "joint"))
            return 0;
        dJointSetAMotorAxis(j, args[1].asInt(), args[2].asInt(),
                            (dReal)args[3].asNumber(), (dReal)args[4].asNumber(), (dReal)args[5].asNumber());
        return 0;
    }

    static int native_dJointSetAMotorParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetAMotorParam expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetAMotorParam", "joint"))
            return 0;
        dJointSetAMotorParam(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetAMotorMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetAMotorMode expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetAMotorMode", "joint"))
            return push_nil1(vm);
        vm->pushInt(dJointGetAMotorMode(j));
        return 1;
    }

    static int native_dJointGetAMotorNumAxes(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetAMotorNumAxes expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetAMotorNumAxes", "joint"))
            return push_nil1(vm);
        vm->pushInt(dJointGetAMotorNumAxes(j));
        return 1;
    }

    static int native_dJointGetAMotorAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetAMotorAxis expects (joint, anum)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetAMotorAxis", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetAMotorAxis(j, args[1].asInt(), v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetAMotorAxisRel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetAMotorAxisRel expects (joint, anum)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetAMotorAxisRel", "joint"))
            return push_nil1(vm);
        vm->pushInt(dJointGetAMotorAxisRel(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointGetAMotorAngle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetAMotorAngle expects (joint, anum)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetAMotorAngle", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetAMotorAngle(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointGetAMotorAngleRate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetAMotorAngleRate expects (joint, anum)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetAMotorAngleRate", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetAMotorAngleRate(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointGetAMotorParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetAMotorParam expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetAMotorParam", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetAMotorParam(j, args[1].asInt()));
        return 1;
    }

    static int native_dJointAddAMotorTorques(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dJointAddAMotorTorques expects (joint, torque1, torque2, torque3)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointAddAMotorTorques", "joint"))
            return 0;
        dJointAddAMotorTorques(j,
                               (dReal)args[1].asNumber(),
                               (dReal)args[2].asNumber(),
                               (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dJointSetLMotorNumAxes(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointSetLMotorNumAxes expects (joint, numAxes)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetLMotorNumAxes", "joint"))
            return 0;
        dJointSetLMotorNumAxes(j, args[1].asInt());
        return 0;
    }

    static int native_dJointSetLMotorAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("dJointSetLMotorAxis expects (joint, anum, rel, x, y, z)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetLMotorAxis", "joint"))
            return 0;
        dJointSetLMotorAxis(j, args[1].asInt(), args[2].asInt(),
                            (dReal)args[3].asNumber(), (dReal)args[4].asNumber(), (dReal)args[5].asNumber());
        return 0;
    }

    static int native_dJointSetLMotorParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dJointSetLMotorParam expects (joint, parameter, value)");
            return 0;
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointSetLMotorParam", "joint"))
            return 0;
        dJointSetLMotorParam(j, args[1].asInt(), (dReal)args[2].asNumber());
        return 0;
    }

    static int native_dJointGetLMotorNumAxes(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dJointGetLMotorNumAxes expects (joint)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetLMotorNumAxes", "joint"))
            return push_nil1(vm);
        vm->pushInt(dJointGetLMotorNumAxes(j));
        return 1;
    }

    static int native_dJointGetLMotorAxis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetLMotorAxis expects (joint, anum)");
            return push_nil3(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetLMotorAxis", "joint"))
            return push_nil3(vm);
        dVector3 v = {0, 0, 0, 0};
        dJointGetLMotorAxis(j, args[1].asInt(), v);
        return push_dvec3(vm, v);
    }

    static int native_dJointGetLMotorParam(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dJointGetLMotorParam expects (joint, parameter)");
            return push_nil1(vm);
        }
        dJointID j = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&j, "dJointGetLMotorParam", "joint"))
            return push_nil1(vm);
        vm->pushDouble((double)dJointGetLMotorParam(j, args[1].asInt()));
        return 1;
    }

    // ---------------- Body ----------------

    static int native_dBodyCreate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyCreate expects (world)");
            return push_nil1(vm);
        }
        dWorldID w = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&w, "dBodyCreate", "world"))
            return push_nil1(vm);
        dBodyID b = dBodyCreate(w);
        vm->pushPointer((void *)b);
        return 1;
    }

    static int native_dBodyDestroy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyDestroy expects (body)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyDestroy", "body"))
            return 0;
        dBodyDestroy(b);
        return 0;
    }

    static int native_dBodySetPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dBodySetPosition expects (body, x, y, z)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetPosition", "body"))
            return 0;
        dBodySetPosition(b, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dBodyGetPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyGetPosition expects (body)");
            return push_nil3(vm);
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyGetPosition", "body"))
            return push_nil3(vm);
        const dReal *p = dBodyGetPosition(b);
        vm->pushDouble((double)p[0]);
        vm->pushDouble((double)p[1]);
        vm->pushDouble((double)p[2]);
        return 3;
    }

    static int native_dBodyGetRotation(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyGetRotation expects (body)");
            return push_nil12(vm);
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyGetRotation", "body"))
            return push_nil12(vm);
        const dReal *r = dBodyGetRotation(b);
        if (!r)
            return push_nil12(vm);
        for (int i = 0; i < 12; ++i)
            vm->pushDouble((double)r[i]);
        return 12;
    }

    static int native_dBodySetLinearVel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dBodySetLinearVel expects (body, x, y, z)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetLinearVel", "body"))
            return 0;
        dBodySetLinearVel(b, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dBodyGetLinearVel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyGetLinearVel expects (body)");
            return push_nil3(vm);
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyGetLinearVel", "body"))
            return push_nil3(vm);
        const dReal *v = dBodyGetLinearVel(b);
        vm->pushDouble((double)v[0]);
        vm->pushDouble((double)v[1]);
        vm->pushDouble((double)v[2]);
        return 3;
    }

    static int native_dBodySetAngularVel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dBodySetAngularVel expects (body, x, y, z)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetAngularVel", "body"))
            return 0;
        dBodySetAngularVel(b, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dBodyGetAngularVel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyGetAngularVel expects (body)");
            return push_nil3(vm);
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyGetAngularVel", "body"))
            return push_nil3(vm);
        const dReal *v = dBodyGetAngularVel(b);
        vm->pushDouble((double)v[0]);
        vm->pushDouble((double)v[1]);
        vm->pushDouble((double)v[2]);
        return 3;
    }

    static int native_dBodyAddForce(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dBodyAddForce expects (body, x, y, z)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyAddForce", "body"))
            return 0;
        dBodyAddForce(b, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dBodyAddTorque(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dBodyAddTorque expects (body, x, y, z)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyAddTorque", "body"))
            return 0;
        dBodyAddTorque(b, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dBodySetKinematic(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodySetKinematic expects (body)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetKinematic", "body"))
            return 0;
        dBodySetKinematic(b);
        return 0;
    }

    static int native_dBodySetDynamic(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodySetDynamic expects (body)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetDynamic", "body"))
            return 0;
        dBodySetDynamic(b);
        return 0;
    }

    static int native_dBodyIsKinematic(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyIsKinematic expects (body)");
            return push_nil1(vm);
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyIsKinematic", "body"))
            return push_nil1(vm);
        vm->pushBool(dBodyIsKinematic(b) != 0);
        return 1;
    }

    static int native_dBodySetAutoDisableFlag(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dBodySetAutoDisableFlag expects (body, flag)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetAutoDisableFlag", "body"))
            return 0;
        dBodySetAutoDisableFlag(b, args[1].asInt());
        return 0;
    }

    static int native_dBodyGetAutoDisableFlag(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyGetAutoDisableFlag expects (body)");
            return push_nil1(vm);
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyGetAutoDisableFlag", "body"))
            return push_nil1(vm);
        vm->pushInt(dBodyGetAutoDisableFlag(b));
        return 1;
    }

    static int native_dBodySetGravityMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dBodySetGravityMode expects (body, mode)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetGravityMode", "body"))
            return 0;
        dBodySetGravityMode(b, args[1].asInt());
        return 0;
    }

    static int native_dBodyGetGravityMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dBodyGetGravityMode expects (body)");
            return push_nil1(vm);
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodyGetGravityMode", "body"))
            return push_nil1(vm);
        vm->pushInt(dBodyGetGravityMode(b));
        return 1;
    }

    // C-style convenience mass setters (mass object is internal here).
    static int native_dBodySetMassBoxTotal(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("dBodySetMassBoxTotal expects (body, totalMass, lx, ly, lz)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetMassBoxTotal", "body"))
            return 0;
        dMass m;
        dMassSetZero(&m);
        dMassSetBoxTotal(&m,
                         (dReal)args[1].asNumber(),
                         (dReal)args[2].asNumber(),
                         (dReal)args[3].asNumber(),
                         (dReal)args[4].asNumber());
        dBodySetMass(b, &m);
        return 0;
    }

    static int native_dBodySetMassSphereTotal(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dBodySetMassSphereTotal expects (body, totalMass, radius)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetMassSphereTotal", "body"))
            return 0;
        dMass m;
        dMassSetZero(&m);
        dMassSetSphereTotal(&m, (dReal)args[1].asNumber(), (dReal)args[2].asNumber());
        dBodySetMass(b, &m);
        return 0;
    }

    static int native_dBodySetMassCapsuleTotal(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("dBodySetMassCapsuleTotal expects (body, totalMass, direction, radius, length)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetMassCapsuleTotal", "body"))
            return 0;
        dMass m;
        dMassSetZero(&m);
        dMassSetCapsuleTotal(&m,
                             (dReal)args[1].asNumber(),
                             args[2].asInt(),
                             (dReal)args[3].asNumber(),
                             (dReal)args[4].asNumber());
        dBodySetMass(b, &m);
        return 0;
    }

    static int native_dBodySetMassCylinderTotal(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("dBodySetMassCylinderTotal expects (body, totalMass, direction, radius, length)");
            return 0;
        }
        dBodyID b = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&b, "dBodySetMassCylinderTotal", "body"))
            return 0;
        dMass m;
        dMassSetZero(&m);
        dMassSetCylinderTotal(&m,
                              (dReal)args[1].asNumber(),
                              args[2].asInt(),
                              (dReal)args[3].asNumber(),
                              (dReal)args[4].asNumber());
        dBodySetMass(b, &m);
        return 0;
    }

    // ---------------- Geom ----------------

    static int native_dCreatePlane(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("dCreatePlane expects (space, a, b, c, d)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dCreatePlane", "space"))
            return push_nil1(vm);
        dGeomID g = dCreatePlane(s,
                                 (dReal)args[1].asNumber(),
                                 (dReal)args[2].asNumber(),
                                 (dReal)args[3].asNumber(),
                                 (dReal)args[4].asNumber());
        vm->pushPointer((void *)g);
        return 1;
    }

    static int native_dCreateBox(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dCreateBox expects (space, lx, ly, lz)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dCreateBox", "space"))
            return push_nil1(vm);
        dGeomID g = dCreateBox(s,
                               (dReal)args[1].asNumber(),
                               (dReal)args[2].asNumber(),
                               (dReal)args[3].asNumber());
        vm->pushPointer((void *)g);
        return 1;
    }

    static int native_dCreateSphere(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dCreateSphere expects (space, radius)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dCreateSphere", "space"))
            return push_nil1(vm);
        dGeomID g = dCreateSphere(s, (dReal)args[1].asNumber());
        vm->pushPointer((void *)g);
        return 1;
    }

    static int native_dCreateCapsule(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dCreateCapsule expects (space, radius, length)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dCreateCapsule", "space"))
            return push_nil1(vm);
        dGeomID g = dCreateCapsule(s, (dReal)args[1].asNumber(), (dReal)args[2].asNumber());
        vm->pushPointer((void *)g);
        return 1;
    }

    static int native_dCreateCylinder(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("dCreateCylinder expects (space, radius, length)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dCreateCylinder", "space"))
            return push_nil1(vm);
        dGeomID g = dCreateCylinder(s, (dReal)args[1].asNumber(), (dReal)args[2].asNumber());
        vm->pushPointer((void *)g);
        return 1;
    }

    static int native_dCreateRay(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dCreateRay expects (space, length)");
            return push_nil1(vm);
        }
        dSpaceID s = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&s, "dCreateRay", "space"))
            return push_nil1(vm);
        dGeomID g = dCreateRay(s, (dReal)args[1].asNumber());
        vm->pushPointer((void *)g);
        return 1;
    }

    static int native_dGeomDestroy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomDestroy expects (geom)");
            return 0;
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomDestroy", "geom"))
            return 0;
        dGeomDestroy(g);
        return 0;
    }

    static int native_dGeomSetBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dGeomSetBody expects (geom, bodyOrNil)");
            return 0;
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomSetBody", "geom"))
            return 0;
        dBodyID b = nullptr;
        if (!read_body_or_nil_arg(args, 1, &b, "dGeomSetBody"))
            return 0;
        dGeomSetBody(g, b);
        return 0;
    }

    static int native_dGeomGetBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomGetBody expects (geom)");
            return push_nil1(vm);
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomGetBody", "geom"))
            return push_nil1(vm);
        dBodyID b = dGeomGetBody(g);
        vm->pushPointer((void *)b);
        return 1;
    }

    static int native_dGeomSetPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("dGeomSetPosition expects (geom, x, y, z)");
            return 0;
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomSetPosition", "geom"))
            return 0;
        dGeomSetPosition(g, (dReal)args[1].asNumber(), (dReal)args[2].asNumber(), (dReal)args[3].asNumber());
        return 0;
    }

    static int native_dGeomGetPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomGetPosition expects (geom)");
            return push_nil3(vm);
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomGetPosition", "geom"))
            return push_nil3(vm);
        const dReal *p = dGeomGetPosition(g);
        if (!p)
            return push_nil3(vm);
        vm->pushDouble((double)p[0]);
        vm->pushDouble((double)p[1]);
        vm->pushDouble((double)p[2]);
        return 3;
    }

    static int native_dGeomGetClass(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomGetClass expects (geom)");
            return push_nil1(vm);
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomGetClass", "geom"))
            return push_nil1(vm);
        vm->pushInt(dGeomGetClass(g));
        return 1;
    }

    static int native_dGeomSetCategoryBits(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dGeomSetCategoryBits expects (geom, bits)");
            return 0;
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomSetCategoryBits", "geom"))
            return 0;
        dGeomSetCategoryBits(g, (unsigned long)args[1].asInt());
        return 0;
    }

    static int native_dGeomGetCategoryBits(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomGetCategoryBits expects (geom)");
            return push_nil1(vm);
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomGetCategoryBits", "geom"))
            return push_nil1(vm);
        vm->pushInt((int)dGeomGetCategoryBits(g));
        return 1;
    }

    static int native_dGeomSetCollideBits(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("dGeomSetCollideBits expects (geom, bits)");
            return 0;
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomSetCollideBits", "geom"))
            return 0;
        dGeomSetCollideBits(g, (unsigned long)args[1].asInt());
        return 0;
    }

    static int native_dGeomGetCollideBits(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomGetCollideBits expects (geom)");
            return push_nil1(vm);
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomGetCollideBits", "geom"))
            return push_nil1(vm);
        vm->pushInt((int)dGeomGetCollideBits(g));
        return 1;
    }

    static int native_dGeomEnable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomEnable expects (geom)");
            return 0;
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomEnable", "geom"))
            return 0;
        dGeomEnable(g);
        return 0;
    }

    static int native_dGeomDisable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomDisable expects (geom)");
            return 0;
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomDisable", "geom"))
            return 0;
        dGeomDisable(g);
        return 0;
    }

    static int native_dGeomIsEnabled(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("dGeomIsEnabled expects (geom)");
            return push_nil1(vm);
        }
        dGeomID g = nullptr;
        if (!read_pointer_arg(args, 0, (void **)&g, "dGeomIsEnabled", "geom"))
            return push_nil1(vm);
        vm->pushInt(dGeomIsEnabled(g));
        return 1;
    }

    void register_ode(Interpreter &vm)
    {
        ModuleBuilder module = vm.addModule("ODE");
        module
            .addFunction("dInitODE2", native_dInitODE2, -1)
            .addFunction("dCloseODE", native_dCloseODE, 0)

            .addFunction("dWorldCreate", native_dWorldCreate, 0)
            .addFunction("dWorldDestroy", native_dWorldDestroy, 1)
            .addFunction("dWorldSetGravity", native_dWorldSetGravity, 4)
            .addFunction("dWorldGetGravity", native_dWorldGetGravity, 1)
            .addFunction("dWorldSetERP", native_dWorldSetERP, 2)
            .addFunction("dWorldGetERP", native_dWorldGetERP, 1)
            .addFunction("dWorldSetCFM", native_dWorldSetCFM, 2)
            .addFunction("dWorldGetCFM", native_dWorldGetCFM, 1)
            .addFunction("dWorldSetQuickStepNumIterations", native_dWorldSetQuickStepNumIterations, 2)
            .addFunction("dWorldGetQuickStepNumIterations", native_dWorldGetQuickStepNumIterations, 1)
            .addFunction("dWorldStep", native_dWorldStep, 2)
            .addFunction("dWorldQuickStep", native_dWorldQuickStep, 2)

            .addFunction("dSimpleSpaceCreate", native_dSimpleSpaceCreate, 1)
            .addFunction("dHashSpaceCreate", native_dHashSpaceCreate, 1)
            .addFunction("dSpaceDestroy", native_dSpaceDestroy, 1)
            .addFunction("dSpaceSetSublevel", native_dSpaceSetSublevel, 2)
            .addFunction("dSpaceGetSublevel", native_dSpaceGetSublevel, 1)
            .addFunction("dSpaceSetCleanup", native_dSpaceSetCleanup, 2)
            .addFunction("dSpaceGetCleanup", native_dSpaceGetCleanup, 1)
            .addFunction("dSpaceAdd", native_dSpaceAdd, 2)
            .addFunction("dSpaceRemove", native_dSpaceRemove, 2)
            .addFunction("dSpaceCollideSimple", native_dSpaceCollideSimple, -1)
            .addFunction("dContactEventsClear", native_dContactEventsClear, 0)
            .addFunction("dContactEventsCount", native_dContactEventsCount, 0)
            .addFunction("dContactEventGet", native_dContactEventGet, 1)

            .addFunction("dJointGroupCreate", native_dJointGroupCreate, -1)
            .addFunction("dJointGroupDestroy", native_dJointGroupDestroy, 1)
            .addFunction("dJointGroupEmpty", native_dJointGroupEmpty, 1)

            .addFunction("dJointCreateBall", native_dJointCreateBall, 2)
            .addFunction("dJointCreateHinge", native_dJointCreateHinge, 2)
            .addFunction("dJointCreateSlider", native_dJointCreateSlider, 2)
            .addFunction("dJointCreateHinge2", native_dJointCreateHinge2, 2)
            .addFunction("dJointCreateUniversal", native_dJointCreateUniversal, 2)
            .addFunction("dJointCreateFixed", native_dJointCreateFixed, 2)
            .addFunction("dJointCreateNull", native_dJointCreateNull, 2)
            .addFunction("dJointCreateAMotor", native_dJointCreateAMotor, 2)
            .addFunction("dJointCreateLMotor", native_dJointCreateLMotor, 2)
            .addFunction("dJointCreatePR", native_dJointCreatePR, 2)
            .addFunction("dJointCreatePU", native_dJointCreatePU, 2)
            .addFunction("dJointCreatePiston", native_dJointCreatePiston, 2)
            .addFunction("dJointCreatePlane2D", native_dJointCreatePlane2D, 2)
            .addFunction("dJointDestroy", native_dJointDestroy, 1)
            .addFunction("dJointAttach", native_dJointAttach, 3)
            .addFunction("dJointEnable", native_dJointEnable, 1)
            .addFunction("dJointDisable", native_dJointDisable, 1)
            .addFunction("dJointIsEnabled", native_dJointIsEnabled, 1)
            .addFunction("dJointGetType", native_dJointGetType, 1)
            .addFunction("dJointGetBody", native_dJointGetBody, 2)
            .addFunction("dAreConnected", native_dAreConnected, 2)
            .addFunction("dAreConnectedExcluding", native_dAreConnectedExcluding, 3)
            .addFunction("dJointSetBallAnchor", native_dJointSetBallAnchor, 4)
            .addFunction("dJointGetBallAnchor", native_dJointGetBallAnchor, 1)
            .addFunction("dJointGetBallAnchor2", native_dJointGetBallAnchor2, 1)
            .addFunction("dJointSetBallParam", native_dJointSetBallParam, 3)
            .addFunction("dJointGetBallParam", native_dJointGetBallParam, 2)
            .addFunction("dJointSetHingeAnchor", native_dJointSetHingeAnchor, 4)
            .addFunction("dJointSetHingeAxis", native_dJointSetHingeAxis, 4)
            .addFunction("dJointGetHingeAnchor", native_dJointGetHingeAnchor, 1)
            .addFunction("dJointGetHingeAnchor2", native_dJointGetHingeAnchor2, 1)
            .addFunction("dJointGetHingeAxis", native_dJointGetHingeAxis, 1)
            .addFunction("dJointGetHingeAngle", native_dJointGetHingeAngle, 1)
            .addFunction("dJointGetHingeAngleRate", native_dJointGetHingeAngleRate, 1)
            .addFunction("dJointSetHingeParam", native_dJointSetHingeParam, 3)
            .addFunction("dJointGetHingeParam", native_dJointGetHingeParam, 2)
            .addFunction("dJointSetSliderAxis", native_dJointSetSliderAxis, 4)
            .addFunction("dJointGetSliderAxis", native_dJointGetSliderAxis, 1)
            .addFunction("dJointGetSliderPosition", native_dJointGetSliderPosition, 1)
            .addFunction("dJointGetSliderPositionRate", native_dJointGetSliderPositionRate, 1)
            .addFunction("dJointSetSliderParam", native_dJointSetSliderParam, 3)
            .addFunction("dJointGetSliderParam", native_dJointGetSliderParam, 2)
            .addFunction("dJointSetHinge2Anchor", native_dJointSetHinge2Anchor, 4)
            .addFunction("dJointSetHinge2Axis1", native_dJointSetHinge2Axis1, 4)
            .addFunction("dJointSetHinge2Axis2", native_dJointSetHinge2Axis2, 4)
            .addFunction("dJointSetHinge2Param", native_dJointSetHinge2Param, 3)
            .addFunction("dJointGetHinge2Anchor", native_dJointGetHinge2Anchor, 1)
            .addFunction("dJointGetHinge2Anchor2", native_dJointGetHinge2Anchor2, 1)
            .addFunction("dJointGetHinge2Axis1", native_dJointGetHinge2Axis1, 1)
            .addFunction("dJointGetHinge2Axis2", native_dJointGetHinge2Axis2, 1)
            .addFunction("dJointGetHinge2Param", native_dJointGetHinge2Param, 2)
            .addFunction("dJointGetHinge2Angle1", native_dJointGetHinge2Angle1, 1)
            .addFunction("dJointGetHinge2Angle2", native_dJointGetHinge2Angle2, 1)
            .addFunction("dJointGetHinge2Angle1Rate", native_dJointGetHinge2Angle1Rate, 1)
            .addFunction("dJointGetHinge2Angle2Rate", native_dJointGetHinge2Angle2Rate, 1)
            .addFunction("dJointSetUniversalAnchor", native_dJointSetUniversalAnchor, 4)
            .addFunction("dJointSetUniversalAxis1", native_dJointSetUniversalAxis1, 4)
            .addFunction("dJointSetUniversalAxis2", native_dJointSetUniversalAxis2, 4)
            .addFunction("dJointSetUniversalParam", native_dJointSetUniversalParam, 3)
            .addFunction("dJointGetUniversalAnchor", native_dJointGetUniversalAnchor, 1)
            .addFunction("dJointGetUniversalAnchor2", native_dJointGetUniversalAnchor2, 1)
            .addFunction("dJointGetUniversalAxis1", native_dJointGetUniversalAxis1, 1)
            .addFunction("dJointGetUniversalAxis2", native_dJointGetUniversalAxis2, 1)
            .addFunction("dJointGetUniversalParam", native_dJointGetUniversalParam, 2)
            .addFunction("dJointGetUniversalAngles", native_dJointGetUniversalAngles, 1)
            .addFunction("dJointGetUniversalAngle1", native_dJointGetUniversalAngle1, 1)
            .addFunction("dJointGetUniversalAngle2", native_dJointGetUniversalAngle2, 1)
            .addFunction("dJointGetUniversalAngle1Rate", native_dJointGetUniversalAngle1Rate, 1)
            .addFunction("dJointGetUniversalAngle2Rate", native_dJointGetUniversalAngle2Rate, 1)
            .addFunction("dJointSetFixed", native_dJointSetFixed, 1)
            .addFunction("dJointSetFixedParam", native_dJointSetFixedParam, 3)
            .addFunction("dJointGetFixedParam", native_dJointGetFixedParam, 2)
            .addFunction("dJointSetAMotorMode", native_dJointSetAMotorMode, 2)
            .addFunction("dJointSetAMotorNumAxes", native_dJointSetAMotorNumAxes, 2)
            .addFunction("dJointSetAMotorAxis", native_dJointSetAMotorAxis, 6)
            .addFunction("dJointSetAMotorParam", native_dJointSetAMotorParam, 3)
            .addFunction("dJointGetAMotorMode", native_dJointGetAMotorMode, 1)
            .addFunction("dJointGetAMotorNumAxes", native_dJointGetAMotorNumAxes, 1)
            .addFunction("dJointGetAMotorAxis", native_dJointGetAMotorAxis, 2)
            .addFunction("dJointGetAMotorAxisRel", native_dJointGetAMotorAxisRel, 2)
            .addFunction("dJointGetAMotorAngle", native_dJointGetAMotorAngle, 2)
            .addFunction("dJointGetAMotorAngleRate", native_dJointGetAMotorAngleRate, 2)
            .addFunction("dJointGetAMotorParam", native_dJointGetAMotorParam, 2)
            .addFunction("dJointAddAMotorTorques", native_dJointAddAMotorTorques, 4)
            .addFunction("dJointSetLMotorNumAxes", native_dJointSetLMotorNumAxes, 2)
            .addFunction("dJointSetLMotorAxis", native_dJointSetLMotorAxis, 6)
            .addFunction("dJointSetLMotorParam", native_dJointSetLMotorParam, 3)
            .addFunction("dJointGetLMotorNumAxes", native_dJointGetLMotorNumAxes, 1)
            .addFunction("dJointGetLMotorAxis", native_dJointGetLMotorAxis, 2)
            .addFunction("dJointGetLMotorParam", native_dJointGetLMotorParam, 2)

            .addFunction("dBodyCreate", native_dBodyCreate, 1)
            .addFunction("dBodyDestroy", native_dBodyDestroy, 1)
            .addFunction("dBodySetPosition", native_dBodySetPosition, 4)
            .addFunction("dBodyGetPosition", native_dBodyGetPosition, 1)
            .addFunction("dBodyGetRotation", native_dBodyGetRotation, 1)
            .addFunction("dBodySetLinearVel", native_dBodySetLinearVel, 4)
            .addFunction("dBodyGetLinearVel", native_dBodyGetLinearVel, 1)
            .addFunction("dBodySetAngularVel", native_dBodySetAngularVel, 4)
            .addFunction("dBodyGetAngularVel", native_dBodyGetAngularVel, 1)
            .addFunction("dBodyAddForce", native_dBodyAddForce, 4)
            .addFunction("dBodyAddTorque", native_dBodyAddTorque, 4)
            .addFunction("dBodySetKinematic", native_dBodySetKinematic, 1)
            .addFunction("dBodySetDynamic", native_dBodySetDynamic, 1)
            .addFunction("dBodyIsKinematic", native_dBodyIsKinematic, 1)
            .addFunction("dBodySetAutoDisableFlag", native_dBodySetAutoDisableFlag, 2)
            .addFunction("dBodyGetAutoDisableFlag", native_dBodyGetAutoDisableFlag, 1)
            .addFunction("dBodySetGravityMode", native_dBodySetGravityMode, 2)
            .addFunction("dBodyGetGravityMode", native_dBodyGetGravityMode, 1)
            .addFunction("dBodySetMassBoxTotal", native_dBodySetMassBoxTotal, 5)
            .addFunction("dBodySetMassSphereTotal", native_dBodySetMassSphereTotal, 3)
            .addFunction("dBodySetMassCapsuleTotal", native_dBodySetMassCapsuleTotal, 5)
            .addFunction("dBodySetMassCylinderTotal", native_dBodySetMassCylinderTotal, 5)

            .addFunction("dCreatePlane", native_dCreatePlane, 5)
            .addFunction("dCreateBox", native_dCreateBox, 4)
            .addFunction("dCreateSphere", native_dCreateSphere, 2)
            .addFunction("dCreateCapsule", native_dCreateCapsule, 3)
            .addFunction("dCreateCylinder", native_dCreateCylinder, 3)
            .addFunction("dCreateRay", native_dCreateRay, 2)
            .addFunction("dGeomDestroy", native_dGeomDestroy, 1)
            .addFunction("dGeomSetBody", native_dGeomSetBody, 2)
            .addFunction("dGeomGetBody", native_dGeomGetBody, 1)
            .addFunction("dGeomSetPosition", native_dGeomSetPosition, 4)
            .addFunction("dGeomGetPosition", native_dGeomGetPosition, 1)
            .addFunction("dGeomGetClass", native_dGeomGetClass, 1)
            .addFunction("dGeomSetCategoryBits", native_dGeomSetCategoryBits, 2)
            .addFunction("dGeomGetCategoryBits", native_dGeomGetCategoryBits, 1)
            .addFunction("dGeomSetCollideBits", native_dGeomSetCollideBits, 2)
            .addFunction("dGeomGetCollideBits", native_dGeomGetCollideBits, 1)
            .addFunction("dGeomEnable", native_dGeomEnable, 1)
            .addFunction("dGeomDisable", native_dGeomDisable, 1)
            .addFunction("dGeomIsEnabled", native_dGeomIsEnabled, 1)

            .addInt("dSphereClass", dSphereClass)
            .addInt("dBoxClass", dBoxClass)
            .addInt("dCapsuleClass", dCapsuleClass)
            .addInt("dCylinderClass", dCylinderClass)
            .addInt("dPlaneClass", dPlaneClass)
            .addInt("dRayClass", dRayClass)
            .addInt("dContactEventsMax", kODEContactEventsMax)
            .addInt("dContactApprox1", dContactApprox1)
            .addInt("dContactBounce", dContactBounce)
            .addInt("dContactSoftERP", dContactSoftERP)
            .addInt("dContactSoftCFM", dContactSoftCFM)
            .addInt("dJointTypeNone", dJointTypeNone)
            .addInt("dJointTypeBall", dJointTypeBall)
            .addInt("dJointTypeHinge", dJointTypeHinge)
            .addInt("dJointTypeSlider", dJointTypeSlider)
            .addInt("dJointTypeContact", dJointTypeContact)
            .addInt("dJointTypeUniversal", dJointTypeUniversal)
            .addInt("dJointTypeHinge2", dJointTypeHinge2)
            .addInt("dJointTypeFixed", dJointTypeFixed)
            .addInt("dJointTypeNull", dJointTypeNull)
            .addInt("dJointTypeAMotor", dJointTypeAMotor)
            .addInt("dJointTypeLMotor", dJointTypeLMotor)
            .addInt("dJointTypePlane2D", dJointTypePlane2D)
            .addInt("dJointTypePR", dJointTypePR)
            .addInt("dJointTypePU", dJointTypePU)
            .addInt("dJointTypePiston", dJointTypePiston)
            .addInt("dParamLoStop", dParamLoStop)
            .addInt("dParamHiStop", dParamHiStop)
            .addInt("dParamVel", dParamVel)
            .addInt("dParamFMax", dParamFMax)
            .addInt("dParamBounce", dParamBounce)
            .addInt("dParamCFM", dParamCFM)
            .addInt("dParamStopERP", dParamStopERP)
            .addInt("dParamStopCFM", dParamStopCFM)
            .addInt("dParamSuspensionERP", dParamSuspensionERP)
            .addInt("dParamSuspensionCFM", dParamSuspensionCFM)
            .addInt("dParamLoStop2", dParamLoStop2)
            .addInt("dParamHiStop2", dParamHiStop2)
            .addInt("dParamVel2", dParamVel2)
            .addInt("dParamFMax2", dParamFMax2)
            .addInt("dParamBounce2", dParamBounce2)
            .addInt("dParamCFM2", dParamCFM2)
            .addInt("dParamStopERP2", dParamStopERP2)
            .addInt("dParamStopCFM2", dParamStopCFM2)
            .addInt("dParamLoStop3", dParamLoStop3)
            .addInt("dParamHiStop3", dParamHiStop3)
            .addInt("dParamVel3", dParamVel3)
            .addInt("dParamFMax3", dParamFMax3)
            .addInt("dParamBounce3", dParamBounce3)
            .addInt("dParamCFM3", dParamCFM3)
            .addInt("dParamStopERP3", dParamStopERP3)
            .addInt("dParamStopCFM3", dParamStopCFM3)
            .addInt("dAMotorUser", dAMotorUser)
            .addInt("dAMotorEuler", dAMotorEuler)
            .addDouble("dInfinity", (double)dInfinity);
    }
        void registerAll(Interpreter &vm)
    {
        register_ode(vm);
    }
}
