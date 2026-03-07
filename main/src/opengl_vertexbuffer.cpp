#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include <vector>

namespace SDLBindings
{
    static bool resolveUploadDataArg(const Value &value,
                                     GLsizeiptr size,
                                     const char *fnName,
                                     const GLvoid **outPtr,
                                     std::vector<unsigned char> &scratch)
    {
        if (value.isNil())
        {
            *outPtr = nullptr;
            return true;
        }

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
        {
            return true;
        }

        if (value.isArray())
        {
            ArrayInstance *arr = value.asArray();
            if (!arr)
            {
                Error("%s received invalid array", fnName);
                return false;
            }

            const int count = (int)arr->values.size();
            if (count <= 0 || size <= 0)
            {
                *outPtr = nullptr;
                return true;
            }

            if (size == (GLsizeiptr)count)
            {
                scratch.resize((size_t)count);
                for (int i = 0; i < count; i++)
                {
                    if (!arr->values[i].isNumber())
                    {
                        Error("%s array must contain only numeric values", fnName);
                        return false;
                    }
                    scratch[(size_t)i] = (unsigned char)arr->values[i].asInt();
                }
                *outPtr = scratch.data();
                return true;
            }

            if (size == (GLsizeiptr)(count * (int)sizeof(float)))
            {
                scratch.resize((size_t)count * sizeof(float));
                float *out = (float *)scratch.data();
                for (int i = 0; i < count; i++)
                {
                    if (!arr->values[i].isNumber())
                    {
                        Error("%s array must contain only numeric values", fnName);
                        return false;
                    }
                    out[i] = (float)arr->values[i].asNumber();
                }
                *outPtr = scratch.data();
                return true;
            }

            if (size == (GLsizeiptr)(count * (int)sizeof(double)))
            {
                scratch.resize((size_t)count * sizeof(double));
                double *out = (double *)scratch.data();
                for (int i = 0; i < count; i++)
                {
                    if (!arr->values[i].isNumber())
                    {
                        Error("%s array must contain only numeric values", fnName);
                        return false;
                    }
                    out[i] = arr->values[i].asNumber();
                }
                *outPtr = scratch.data();
                return true;
            }

            Error("%s array size mismatch: expected bytes = count, count*4 or count*8", fnName);
            return false;
        }

        Error("%s expects pointer, buffer, typedarray, array or nil", fnName);
        return false;
    }

    int native_glGenBuffers(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenBuffers expects 0 arguments");
            return 0;
        }
        GLuint buffer = 0;
        glGenBuffers(1, &buffer);
        vm->pushInt((int)buffer);
        return 1;
    }

    int native_glBindBuffer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glBindBuffer expects 2 arguments: target, buffer");
            return 0;
        }
        GLenum target = (GLenum)args[0].asNumber();
        GLuint buffer = (GLuint)args[1].asNumber();
        glBindBuffer(target, buffer);
        return 0;
    }

    int native_glBufferData(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glBufferData expects 4 arguments: target, size, data, usage");
            return 0;
        }
        GLenum target = (GLenum)args[0].asNumber();
        GLsizeiptr size = (GLsizeiptr)args[1].asNumber();
        GLenum usage = (GLenum)args[3].asNumber();

        const GLvoid *data = nullptr;
        std::vector<unsigned char> scratch;
        if (!resolveUploadDataArg(args[2], size, "glBufferData", &data, scratch))
            return 0;

        glBufferData(target, size, data, usage);
        return 0;
    }

    int native_glBufferSubData(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glBufferSubData expects 4 arguments: target, offset, size, data");
            return 0;
        }
        GLenum target = (GLenum)args[0].asNumber();
        GLintptr offset = (GLintptr)args[1].asNumber();
        GLsizeiptr size = (GLsizeiptr)args[2].asNumber();

        const GLvoid *data = nullptr;
        std::vector<unsigned char> scratch;
        if (!resolveUploadDataArg(args[3], size, "glBufferSubData", &data, scratch))
            return 0;

        glBufferSubData(target, offset, size, data);
        return 0;
    }

    int native_glMapBuffer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glMapBuffer expects 2 arguments: target, access");
            return 0;
        }
        GLenum target = (GLenum)args[0].asNumber();
        GLenum access = (GLenum)args[1].asNumber();
        void *ptr = glMapBuffer(target, access);
        if (ptr)
            vm->pushPointer(ptr);
        else
            vm->pushNil();
        return 1;
    }

    int native_glUnmapBuffer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glUnmapBuffer expects 1 argument: target");
            return 0;
        }
        GLenum target = (GLenum)args[0].asNumber();
        GLboolean result = glUnmapBuffer(target);
        vm->pushBool(result);
        return 1;
    }

    int native_glDeleteBuffers(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glDeleteBuffers expects 1 argument");
            return 0;
        }
        GLuint buffer = (GLuint)args[0].asNumber();
        glDeleteBuffers(1, &buffer);
        return 0;
    }

    void register_opengl_vertexbuffer(ModuleBuilder &module)
    {
        module.addFunction("glGenBuffers", native_glGenBuffers, 0)
            .addFunction("glBindBuffer", native_glBindBuffer, 2)
            .addFunction("glBufferData", native_glBufferData, 4)
            .addFunction("glBufferSubData", native_glBufferSubData, 4)
            .addFunction("glMapBuffer", native_glMapBuffer, 2)
            .addFunction("glUnmapBuffer", native_glUnmapBuffer, 1)
            .addFunction("glDeleteBuffers", native_glDeleteBuffers, 1)
            .addInt("GL_ARRAY_BUFFER", GL_ARRAY_BUFFER)
            .addInt("GL_ELEMENT_ARRAY_BUFFER", GL_ELEMENT_ARRAY_BUFFER)
            .addInt("GL_STATIC_DRAW", GL_STATIC_DRAW)
            .addInt("GL_DYNAMIC_DRAW", GL_DYNAMIC_DRAW)
            .addInt("GL_STREAM_DRAW", GL_STREAM_DRAW)
            .addInt("GL_READ_ONLY", 0x88B8)
            .addInt("GL_WRITE_ONLY", 0x88B9)
            .addInt("GL_READ_WRITE", 0x88BA);
    }
}
