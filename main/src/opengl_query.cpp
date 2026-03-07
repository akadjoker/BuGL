#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

namespace SDLBindings
{
    static int native_glGenQueries(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenQueries expects 0 arguments");
            return 0;
        }
        GLuint q = 0;
        glGenQueries(1, &q);
        vm->pushInt((int)q);
        return 1;
    }

    static int native_glDeleteQueries(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteQueries expects 1 argument: query");
            return 0;
        }
        GLuint q = (GLuint)args[0].asNumber();
        glDeleteQueries(1, &q);
        return 0;
    }

    static int native_glBeginQuery(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glBeginQuery expects (target, query)");
            return 0;
        }
        glBeginQuery((GLenum)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    static int native_glEndQuery(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glEndQuery expects (target)");
            return 0;
        }
        glEndQuery((GLenum)args[0].asNumber());
        return 0;
    }

    static int native_glGetQueryObjectuiv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glGetQueryObjectuiv expects (query, pname)");
            return 0;
        }
        GLuint value = 0;
        glGetQueryObjectuiv((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), &value);
        vm->pushInt((int)value);
        return 1;
    }

    static int native_glGetQueryObjectiv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glGetQueryObjectiv expects (query, pname)");
            return 0;
        }
        GLint value = 0;
        glGetQueryObjectiv((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), &value);
        vm->pushInt((int)value);
        return 1;
    }

    static int native_glIsQuery(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glIsQuery expects 1 argument: query");
            return 0;
        }
        GLboolean ok = glIsQuery((GLuint)args[0].asNumber());
        vm->pushBool(ok);
        return 1;
    }

    void register_opengl_query(ModuleBuilder &module)
    {
        module.addFunction("glGenQueries", native_glGenQueries, 0)
            .addFunction("glDeleteQueries", native_glDeleteQueries, 1)
            .addFunction("glBeginQuery", native_glBeginQuery, 2)
            .addFunction("glEndQuery", native_glEndQuery, 1)
            .addFunction("glGetQueryObjectuiv", native_glGetQueryObjectuiv, 2)
            .addFunction("glGetQueryObjectiv", native_glGetQueryObjectiv, 2)
            .addFunction("glIsQuery", native_glIsQuery, 1);

#ifdef GL_SAMPLES_PASSED
        module.addInt("GL_SAMPLES_PASSED", GL_SAMPLES_PASSED);
#endif
#ifdef GL_ANY_SAMPLES_PASSED
        module.addInt("GL_ANY_SAMPLES_PASSED", GL_ANY_SAMPLES_PASSED);
#endif
#ifdef GL_ANY_SAMPLES_PASSED_CONSERVATIVE
        module.addInt("GL_ANY_SAMPLES_PASSED_CONSERVATIVE", GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
#endif
#ifdef GL_QUERY_RESULT
        module.addInt("GL_QUERY_RESULT", GL_QUERY_RESULT);
#endif
#ifdef GL_QUERY_RESULT_AVAILABLE
        module.addInt("GL_QUERY_RESULT_AVAILABLE", GL_QUERY_RESULT_AVAILABLE);
#endif
#ifdef GL_QUERY_COUNTER_BITS
        module.addInt("GL_QUERY_COUNTER_BITS", GL_QUERY_COUNTER_BITS);
#endif
#ifdef GL_CURRENT_QUERY
        module.addInt("GL_CURRENT_QUERY", GL_CURRENT_QUERY);
#endif
    }
}
