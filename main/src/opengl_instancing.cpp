#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

namespace SDLBindings
{
    static bool resolveModernPointerArg(const Value &value, const char *fnName, const GLvoid **outPtr)
    {
        if (value.isPointer())
        {
            *outPtr = value.asPointer();
            return true;
        }

        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            *outPtr = buf ? (const GLvoid *)buf->data : nullptr;
            return true;
        }

        if (getBuiltinTypedArrayData(value, (const void **)outPtr))
            return true;

        if (value.isNumber())
        {
            *outPtr = (const GLvoid *)(uintptr_t)value.asNumber();
            return true;
        }

        Error("%s expects pointer/buffer/typedarray or offset number", fnName);
        return false;
    }

    static int native_glDrawArraysInstanced(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glDrawArraysInstanced expects (mode, first, count, instanceCount)");
            return 0;
        }

        glDrawArraysInstanced((GLenum)args[0].asNumber(),
                              (GLint)args[1].asNumber(),
                              (GLsizei)args[2].asNumber(),
                              (GLsizei)args[3].asNumber());
        return 0;
    }

    static int native_glDrawElementsInstanced(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("glDrawElementsInstanced expects (mode, count, type, indicesOrOffset, instanceCount)");
            return 0;
        }

        GLenum mode = (GLenum)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        GLenum type = (GLenum)args[2].asNumber();
        const GLvoid *indices = nullptr;
        if (!resolveModernPointerArg(args[3], "glDrawElementsInstanced", &indices))
            return 0;
        GLsizei instanceCount = (GLsizei)args[4].asNumber();

        glDrawElementsInstanced(mode, count, type, indices, instanceCount);
        return 0;
    }

    static int native_glVertexAttribDivisor(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glVertexAttribDivisor expects (index, divisor)");
            return 0;
        }
        glVertexAttribDivisor((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    void register_opengl_instancing(ModuleBuilder &module)
    {
        module.addFunction("glDrawArraysInstanced", native_glDrawArraysInstanced, 4)
            .addFunction("glDrawElementsInstanced", native_glDrawElementsInstanced, 5)
            .addFunction("glVertexAttribDivisor", native_glVertexAttribDivisor, 2);

#ifdef GL_VERTEX_ATTRIB_ARRAY_DIVISOR
        module.addInt("GL_VERTEX_ATTRIB_ARRAY_DIVISOR", GL_VERTEX_ATTRIB_ARRAY_DIVISOR);
#endif
    }
}
