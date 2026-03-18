#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <vector>
#include <limits>

namespace SDLBindings
{
    // Helpers (same pattern as opengl_shader.cpp)
    static bool resolveAdvPointerArg(const Value &value, const char *fnName, const GLvoid **outPtr)
    {
        if (value.isPointer())   { *outPtr = value.asPointer(); return true; }
        if (value.isBuffer())    { BufferInstance *b = value.asBuffer(); *outPtr = b ? (const GLvoid *)b->data : nullptr; return true; }
        if (getBuiltinTypedArrayData(value, (const void **)outPtr)) return true;
        if (value.isNumber())    { *outPtr = (const GLvoid *)(uintptr_t)value.asNumber(); return true; }
        Error("%s expects pointer/buffer/typedarray or offset number", fnName);
        return false;
    }

    static bool resolveAdvUploadData(const Value &value, GLsizeiptr size, const char *fnName,
                                     const GLvoid **outPtr, std::vector<unsigned char> &scratch)
    {
        if (value.isNil())       { *outPtr = nullptr; return true; }
        if (value.isPointer())   { *outPtr = value.asPointer(); return true; }
        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            *outPtr = buf ? (const GLvoid *)buf->data : nullptr;
            return true;
        }
        if (getBuiltinTypedArrayData(value, (const void **)outPtr)) return true;
        if (value.isArray())
        {
            ArrayInstance *arr = value.asArray();
            if (!arr) { Error("%s received invalid array", fnName); return false; }
            const int count = (int)arr->values.size();
            if (count <= 0 || size <= 0) { *outPtr = nullptr; return true; }
            if (size == (GLsizeiptr)(count * (int)sizeof(float)))
            {
                scratch.resize((size_t)count * sizeof(float));
                float *out = (float *)scratch.data();
                for (int i = 0; i < count; i++)
                {
                    if (!arr->values[i].isNumber()) { Error("%s array must contain numbers", fnName); return false; }
                    out[i] = (float)arr->values[i].asNumber();
                }
                *outPtr = scratch.data();
                return true;
            }
            *outPtr = nullptr;
            return true;
        }
        Error("%s expects pointer, buffer, typedarray, array or nil", fnName);
        return false;
    }

    // =====================================================
    // INDIRECT DRAWING
    // =====================================================

    int native_glDrawArraysIndirect(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glDrawArraysIndirect expects (mode, indirect_offset)");
            return 0;
        }
        glDrawArraysIndirect((GLenum)args[0].asNumber(), (const void *)(uintptr_t)args[1].asNumber());
        return 0;
    }

    int native_glDrawElementsIndirect(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glDrawElementsIndirect expects (mode, type, indirect_offset)");
            return 0;
        }
        glDrawElementsIndirect((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(),
                               (const void *)(uintptr_t)args[2].asNumber());
        return 0;
    }

#ifdef GL_VERSION_4_3
    int native_glMultiDrawArraysIndirect(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glMultiDrawArraysIndirect expects (mode, indirect_offset, drawcount, stride)");
            return 0;
        }
        glMultiDrawArraysIndirect((GLenum)args[0].asNumber(),
                                  (const void *)(uintptr_t)args[1].asNumber(),
                                  (GLsizei)args[2].asNumber(), (GLsizei)args[3].asNumber());
        return 0;
    }

    int native_glMultiDrawElementsIndirect(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glMultiDrawElementsIndirect expects (mode, type, indirect_offset, drawcount, stride)");
            return 0;
        }
        glMultiDrawElementsIndirect((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(),
                                    (const void *)(uintptr_t)args[2].asNumber(),
                                    (GLsizei)args[3].asNumber(), (GLsizei)args[4].asNumber());
        return 0;
    }
#endif

    int native_glDrawRangeElements(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 6)
        {
            Error("glDrawRangeElements expects (mode, start, end, count, type, indices)");
            return 0;
        }
        const GLvoid *indices = nullptr;
        if (!resolveAdvPointerArg(args[5], "glDrawRangeElements", &indices)) return 0;
        glDrawRangeElements((GLenum)args[0].asNumber(), (GLuint)args[1].asNumber(),
                            (GLuint)args[2].asNumber(), (GLsizei)args[3].asNumber(),
                            (GLenum)args[4].asNumber(), indices);
        return 0;
    }

    int native_glDrawElementsBaseVertex(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glDrawElementsBaseVertex expects (mode, count, type, indices, basevertex)");
            return 0;
        }
        const GLvoid *indices = nullptr;
        if (!resolveAdvPointerArg(args[3], "glDrawElementsBaseVertex", &indices)) return 0;
        glDrawElementsBaseVertex((GLenum)args[0].asNumber(), (GLsizei)args[1].asNumber(),
                                 (GLenum)args[2].asNumber(), indices, (GLint)args[4].asNumber());
        return 0;
    }

    int native_glDrawElementsInstancedBaseVertex(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 6)
        {
            Error("glDrawElementsInstancedBaseVertex expects (mode, count, type, indices, instancecount, basevertex)");
            return 0;
        }
        const GLvoid *indices = nullptr;
        if (!resolveAdvPointerArg(args[3], "glDrawElementsInstancedBaseVertex", &indices)) return 0;
        glDrawElementsInstancedBaseVertex((GLenum)args[0].asNumber(), (GLsizei)args[1].asNumber(),
                                          (GLenum)args[2].asNumber(), indices,
                                          (GLsizei)args[4].asNumber(), (GLint)args[5].asNumber());
        return 0;
    }

    // =====================================================
    // SYNC OBJECTS
    // =====================================================

    int native_glFenceSync(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glFenceSync expects (condition, flags)");
            return 0;
        }
        GLsync sync = glFenceSync((GLenum)args[0].asNumber(), (GLbitfield)args[1].asNumber());
        vm->pushPointer((void *)sync);
        return 1;
    }

    int native_glClientWaitSync(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glClientWaitSync expects (sync, flags, timeout_ns)");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("glClientWaitSync: sync must be a pointer");
            return 0;
        }
        GLenum result = glClientWaitSync((GLsync)args[0].asPointer(),
                                         (GLbitfield)args[1].asNumber(),
                                         (GLuint64)args[2].asNumber());
        vm->pushInt((int)result);
        return 1;
    }

    int native_glWaitSync(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glWaitSync expects (sync, flags, timeout)");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("glWaitSync: sync must be a pointer");
            return 0;
        }
        glWaitSync((GLsync)args[0].asPointer(), (GLbitfield)args[1].asNumber(),
                   (GLuint64)args[2].asNumber());
        return 0;
    }

    int native_glDeleteSync(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteSync expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("glDeleteSync: sync must be a pointer");
            return 0;
        }
        glDeleteSync((GLsync)args[0].asPointer());
        return 0;
    }

    int native_glIsSync(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glIsSync expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            vm->pushBool(false);
            return 1;
        }
        GLboolean ok = glIsSync((GLsync)args[0].asPointer());
        vm->pushBool(ok);
        return 1;
    }

    // =====================================================
    // SAMPLER OBJECTS
    // =====================================================

    int native_glGenSamplers(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenSamplers expects 0 arguments");
            return 0;
        }
        GLuint sampler = 0;
        glGenSamplers(1, &sampler);
        vm->pushInt((int)sampler);
        return 1;
    }

    int native_glDeleteSamplers(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteSamplers expects 1 argument");
            return 0;
        }
        GLuint sampler = (GLuint)args[0].asNumber();
        glDeleteSamplers(1, &sampler);
        return 0;
    }

    int native_glBindSampler(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glBindSampler expects 2 arguments (unit, sampler)");
            return 0;
        }
        glBindSampler((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glSamplerParameteri(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glSamplerParameteri expects 3 arguments (sampler, pname, param)");
            return 0;
        }
        glSamplerParameteri((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLint)args[2].asNumber());
        return 0;
    }

    int native_glSamplerParameterf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glSamplerParameterf expects 3 arguments (sampler, pname, param)");
            return 0;
        }
        glSamplerParameterf((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLfloat)args[2].asNumber());
        return 0;
    }

    // =====================================================
    // STATE QUERY
    // =====================================================

    int native_glGetFloatv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetFloatv expects 1 argument");
            return 0;
        }
        GLfloat value = 0.0f;
        glGetFloatv((GLenum)args[0].asNumber(), &value);
        vm->pushFloat(value);
        return 1;
    }

    int native_glGetBooleanv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetBooleanv expects 1 argument");
            return 0;
        }
        GLboolean value = GL_FALSE;
        glGetBooleanv((GLenum)args[0].asNumber(), &value);
        vm->pushBool(value == GL_TRUE);
        return 1;
    }

    int native_glGetDoublev(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetDoublev expects 1 argument");
            return 0;
        }
        GLdouble value = 0.0;
        glGetDoublev((GLenum)args[0].asNumber(), &value);
        vm->pushDouble(value);
        return 1;
    }

    int native_glIsEnabled(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glIsEnabled expects 1 argument");
            return 0;
        }
        GLboolean ok = glIsEnabled((GLenum)args[0].asNumber());
        vm->pushBool(ok);
        return 1;
    }

    // =====================================================
    // BLEND EXTRAS
    // =====================================================

    int native_glBlendColor(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glBlendColor expects 4 arguments (r, g, b, a)");
            return 0;
        }
        glBlendColor((GLfloat)args[0].asNumber(), (GLfloat)args[1].asNumber(),
                     (GLfloat)args[2].asNumber(), (GLfloat)args[3].asNumber());
        return 0;
    }

    // =====================================================
    // CLEAR BUFFER FUNCTIONS
    // =====================================================

    int native_glClearBufferfv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3 || !args[2].isArray())
        {
            Error("glClearBufferfv expects (buffer, drawbuffer, valueArray)");
            return 0;
        }
        GLenum buffer = (GLenum)args[0].asNumber();
        GLint drawbuffer = (GLint)args[1].asNumber();
        ArrayInstance *arr = args[2].asArray();
        if (!arr) { Error("glClearBufferfv: invalid array"); return 0; }

        std::vector<GLfloat> values(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); i++)
            values[i] = (GLfloat)arr->values[i].asNumber();

        glClearBufferfv(buffer, drawbuffer, values.data());
        return 0;
    }

    int native_glClearBufferiv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3 || !args[2].isArray())
        {
            Error("glClearBufferiv expects (buffer, drawbuffer, valueArray)");
            return 0;
        }
        GLenum buffer = (GLenum)args[0].asNumber();
        GLint drawbuffer = (GLint)args[1].asNumber();
        ArrayInstance *arr = args[2].asArray();
        if (!arr) { Error("glClearBufferiv: invalid array"); return 0; }

        std::vector<GLint> values(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); i++)
            values[i] = (GLint)arr->values[i].asNumber();

        glClearBufferiv(buffer, drawbuffer, values.data());
        return 0;
    }

    int native_glClearBufferuiv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3 || !args[2].isArray())
        {
            Error("glClearBufferuiv expects (buffer, drawbuffer, valueArray)");
            return 0;
        }
        GLenum buffer = (GLenum)args[0].asNumber();
        GLint drawbuffer = (GLint)args[1].asNumber();
        ArrayInstance *arr = args[2].asArray();
        if (!arr) { Error("glClearBufferuiv: invalid array"); return 0; }

        std::vector<GLuint> values(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); i++)
            values[i] = (GLuint)arr->values[i].asNumber();

        glClearBufferuiv(buffer, drawbuffer, values.data());
        return 0;
    }

    int native_glClearBufferfi(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glClearBufferfi expects (buffer, drawbuffer, depth, stencil)");
            return 0;
        }
        glClearBufferfi((GLenum)args[0].asNumber(), (GLint)args[1].asNumber(),
                        (GLfloat)args[2].asNumber(), (GLint)args[3].asNumber());
        return 0;
    }

    // =====================================================
    // BUFFER OPERATIONS
    // =====================================================

    int native_glCopyBufferSubData(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glCopyBufferSubData expects (readTarget, writeTarget, readOffset, writeOffset, size)");
            return 0;
        }
        glCopyBufferSubData((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(),
                            (GLintptr)args[2].asNumber(), (GLintptr)args[3].asNumber(),
                            (GLsizeiptr)args[4].asNumber());
        return 0;
    }

    int native_glGetBufferSubData(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glGetBufferSubData expects (target, offset, size, outputBuffer)");
            return 0;
        }
        GLenum target = (GLenum)args[0].asNumber();
        GLintptr offset = (GLintptr)args[1].asNumber();
        GLsizeiptr size = (GLsizeiptr)args[2].asNumber();

        if (!args[3].isBuffer())
        {
            Error("glGetBufferSubData: output must be a buffer");
            return 0;
        }
        BufferInstance *buf = args[3].asBuffer();
        if (!buf || !buf->data)
        {
            Error("glGetBufferSubData: invalid output buffer");
            return 0;
        }
        size_t bufBytes = (size_t)buf->count * (size_t)buf->elementSize;
        if ((size_t)size > bufBytes)
        {
            Error("glGetBufferSubData: output buffer too small");
            return 0;
        }

        glGetBufferSubData(target, offset, size, buf->data);
        return 0;
    }

    int native_glMapBufferRange(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glMapBufferRange expects (target, offset, length, access)");
            return 0;
        }
        void *ptr = glMapBufferRange((GLenum)args[0].asNumber(), (GLintptr)args[1].asNumber(),
                                     (GLsizeiptr)args[2].asNumber(), (GLbitfield)args[3].asNumber());
        if (ptr)
            vm->pushPointer(ptr);
        else
            vm->pushNil();
        return 1;
    }

    int native_glFlushMappedBufferRange(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glFlushMappedBufferRange expects (target, offset, length)");
            return 0;
        }
        glFlushMappedBufferRange((GLenum)args[0].asNumber(), (GLintptr)args[1].asNumber(),
                                 (GLsizeiptr)args[2].asNumber());
        return 0;
    }

    // =====================================================
    // VERTEX ATTRIBUTE EXTRAS
    // =====================================================

    int native_glVertexAttribIPointer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glVertexAttribIPointer expects (index, size, type, stride, pointer)");
            return 0;
        }
        const GLvoid *pointer = nullptr;
        if (!resolveAdvPointerArg(args[4], "glVertexAttribIPointer", &pointer)) return 0;
        glVertexAttribIPointer((GLuint)args[0].asNumber(), (GLint)args[1].asNumber(),
                               (GLenum)args[2].asNumber(), (GLsizei)args[3].asNumber(), pointer);
        return 0;
    }

    int native_glVertexAttrib1f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2) { Error("glVertexAttrib1f expects (index, x)"); return 0; }
        glVertexAttrib1f((GLuint)args[0].asNumber(), (GLfloat)args[1].asNumber());
        return 0;
    }

    int native_glVertexAttrib2f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3) { Error("glVertexAttrib2f expects (index, x, y)"); return 0; }
        glVertexAttrib2f((GLuint)args[0].asNumber(), (GLfloat)args[1].asNumber(), (GLfloat)args[2].asNumber());
        return 0;
    }

    int native_glVertexAttrib3f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4) { Error("glVertexAttrib3f expects (index, x, y, z)"); return 0; }
        glVertexAttrib3f((GLuint)args[0].asNumber(), (GLfloat)args[1].asNumber(),
                         (GLfloat)args[2].asNumber(), (GLfloat)args[3].asNumber());
        return 0;
    }

    int native_glVertexAttrib4f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5) { Error("glVertexAttrib4f expects (index, x, y, z, w)"); return 0; }
        glVertexAttrib4f((GLuint)args[0].asNumber(), (GLfloat)args[1].asNumber(),
                         (GLfloat)args[2].asNumber(), (GLfloat)args[3].asNumber(),
                         (GLfloat)args[4].asNumber());
        return 0;
    }

    // =====================================================
    // UNIFORM EXTRAS (unsigned int)
    // =====================================================

    int native_glUniform1ui(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2) { Error("glUniform1ui expects (location, v0)"); return 0; }
        glUniform1ui((GLint)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glUniform2ui(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3) { Error("glUniform2ui expects (location, v0, v1)"); return 0; }
        glUniform2ui((GLint)args[0].asNumber(), (GLuint)args[1].asNumber(), (GLuint)args[2].asNumber());
        return 0;
    }

    int native_glUniform3ui(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4) { Error("glUniform3ui expects (location, v0, v1, v2)"); return 0; }
        glUniform3ui((GLint)args[0].asNumber(), (GLuint)args[1].asNumber(),
                     (GLuint)args[2].asNumber(), (GLuint)args[3].asNumber());
        return 0;
    }

    int native_glUniform4ui(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5) { Error("glUniform4ui expects (location, v0, v1, v2, v3)"); return 0; }
        glUniform4ui((GLint)args[0].asNumber(), (GLuint)args[1].asNumber(),
                     (GLuint)args[2].asNumber(), (GLuint)args[3].asNumber(),
                     (GLuint)args[4].asNumber());
        return 0;
    }

    // =====================================================
    // FRAMEBUFFER EXTRAS
    // =====================================================

    int native_glFramebufferTextureLayer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glFramebufferTextureLayer expects (target, attachment, texture, level, layer)");
            return 0;
        }
        glFramebufferTextureLayer((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(),
                                  (GLuint)args[2].asNumber(), (GLint)args[3].asNumber(),
                                  (GLint)args[4].asNumber());
        return 0;
    }

    int native_glReadBuffer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glReadBuffer expects 1 argument");
            return 0;
        }
        glReadBuffer((GLenum)args[0].asNumber());
        return 0;
    }

    // =====================================================
    // BUFFER TEXTURE
    // =====================================================

    int native_glTexBuffer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glTexBuffer expects (target, internalformat, buffer)");
            return 0;
        }
        glTexBuffer((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLuint)args[2].asNumber());
        return 0;
    }

    // =====================================================
    // TIMER QUERIES
    // =====================================================

    int native_glQueryCounter(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glQueryCounter expects (id, target)");
            return 0;
        }
        glQueryCounter((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber());
        return 0;
    }

    int native_glGetQueryObjectui64v(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glGetQueryObjectui64v expects (id, pname)");
            return 0;
        }
        GLuint64 value = 0;
        glGetQueryObjectui64v((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), &value);
        vm->pushDouble((double)value);
        return 1;
    }

    // =====================================================
    // REGISTRATION
    // =====================================================

    void register_opengl_advanced(ModuleBuilder &module)
    {
        module
            // Indirect drawing
            .addFunction("glDrawArraysIndirect", native_glDrawArraysIndirect, 2)
            .addFunction("glDrawElementsIndirect", native_glDrawElementsIndirect, 3)
            .addFunction("glDrawRangeElements", native_glDrawRangeElements, 6)
            .addFunction("glDrawElementsBaseVertex", native_glDrawElementsBaseVertex, 5)
            .addFunction("glDrawElementsInstancedBaseVertex", native_glDrawElementsInstancedBaseVertex, 6)

            // Sync objects
            .addFunction("glFenceSync", native_glFenceSync, 2)
            .addFunction("glClientWaitSync", native_glClientWaitSync, 3)
            .addFunction("glWaitSync", native_glWaitSync, 3)
            .addFunction("glDeleteSync", native_glDeleteSync, 1)
            .addFunction("glIsSync", native_glIsSync, 1)

            // Samplers
            .addFunction("glGenSamplers", native_glGenSamplers, 0)
            .addFunction("glDeleteSamplers", native_glDeleteSamplers, 1)
            .addFunction("glBindSampler", native_glBindSampler, 2)
            .addFunction("glSamplerParameteri", native_glSamplerParameteri, 3)
            .addFunction("glSamplerParameterf", native_glSamplerParameterf, 3)

            // State query
            .addFunction("glGetFloatv", native_glGetFloatv, 1)
            .addFunction("glGetBooleanv", native_glGetBooleanv, 1)
            .addFunction("glGetDoublev", native_glGetDoublev, 1)
            .addFunction("glIsEnabled", native_glIsEnabled, 1)

            // Blend extras
            .addFunction("glBlendColor", native_glBlendColor, 4)

            // Clear buffers
            .addFunction("glClearBufferfv", native_glClearBufferfv, 3)
            .addFunction("glClearBufferiv", native_glClearBufferiv, 3)
            .addFunction("glClearBufferuiv", native_glClearBufferuiv, 3)
            .addFunction("glClearBufferfi", native_glClearBufferfi, 4)

            // Buffer ops
            .addFunction("glCopyBufferSubData", native_glCopyBufferSubData, 5)
            .addFunction("glGetBufferSubData", native_glGetBufferSubData, 4)
            .addFunction("glMapBufferRange", native_glMapBufferRange, 4)
            .addFunction("glFlushMappedBufferRange", native_glFlushMappedBufferRange, 3)

            // Vertex attrib extras
            .addFunction("glVertexAttribIPointer", native_glVertexAttribIPointer, 5)
            .addFunction("glVertexAttrib1f", native_glVertexAttrib1f, 2)
            .addFunction("glVertexAttrib2f", native_glVertexAttrib2f, 3)
            .addFunction("glVertexAttrib3f", native_glVertexAttrib3f, 4)
            .addFunction("glVertexAttrib4f", native_glVertexAttrib4f, 5)

            // Uniform uint
            .addFunction("glUniform1ui", native_glUniform1ui, 2)
            .addFunction("glUniform2ui", native_glUniform2ui, 3)
            .addFunction("glUniform3ui", native_glUniform3ui, 4)
            .addFunction("glUniform4ui", native_glUniform4ui, 5)

            // FBO extras
            .addFunction("glFramebufferTextureLayer", native_glFramebufferTextureLayer, 5)
            .addFunction("glReadBuffer", native_glReadBuffer, 1)

            // Buffer textures
            .addFunction("glTexBuffer", native_glTexBuffer, 3)

            // Timer queries
            .addFunction("glQueryCounter", native_glQueryCounter, 2)
            .addFunction("glGetQueryObjectui64v", native_glGetQueryObjectui64v, 2);

#ifdef GL_VERSION_4_3
        module
            .addFunction("glMultiDrawArraysIndirect", native_glMultiDrawArraysIndirect, 4)
            .addFunction("glMultiDrawElementsIndirect", native_glMultiDrawElementsIndirect, 5);
#endif

        // Indirect draw constants
#ifdef GL_DRAW_INDIRECT_BUFFER
        module.addInt("GL_DRAW_INDIRECT_BUFFER", GL_DRAW_INDIRECT_BUFFER);
#endif

        // Sync constants
#ifdef GL_SYNC_GPU_COMMANDS_COMPLETE
        module.addInt("GL_SYNC_GPU_COMMANDS_COMPLETE", GL_SYNC_GPU_COMMANDS_COMPLETE);
#endif
#ifdef GL_SYNC_FLUSH_COMMANDS_BIT
        module.addInt("GL_SYNC_FLUSH_COMMANDS_BIT", GL_SYNC_FLUSH_COMMANDS_BIT);
#endif
#ifdef GL_ALREADY_SIGNALED
        module.addInt("GL_ALREADY_SIGNALED", GL_ALREADY_SIGNALED)
              .addInt("GL_TIMEOUT_EXPIRED", GL_TIMEOUT_EXPIRED)
              .addInt("GL_CONDITION_SATISFIED", GL_CONDITION_SATISFIED)
              .addInt("GL_WAIT_FAILED", GL_WAIT_FAILED);
#endif
#ifdef GL_TIMEOUT_IGNORED
        // GL_TIMEOUT_IGNORED is 0xFFFFFFFFFFFFFFFF, store as -1 (script should use as uint64)
        module.addInt("GL_TIMEOUT_IGNORED", -1);
#endif

        // Sampler/texture compare constants
#ifdef GL_TEXTURE_COMPARE_MODE
        module.addInt("GL_TEXTURE_COMPARE_MODE", GL_TEXTURE_COMPARE_MODE);
#endif
#ifdef GL_TEXTURE_COMPARE_FUNC
        module.addInt("GL_TEXTURE_COMPARE_FUNC", GL_TEXTURE_COMPARE_FUNC);
#endif
#ifdef GL_COMPARE_REF_TO_TEXTURE
        module.addInt("GL_COMPARE_REF_TO_TEXTURE", GL_COMPARE_REF_TO_TEXTURE);
#endif

        // Blend color constants
#ifdef GL_CONSTANT_COLOR
        module.addInt("GL_CONSTANT_COLOR", GL_CONSTANT_COLOR)
              .addInt("GL_ONE_MINUS_CONSTANT_COLOR", GL_ONE_MINUS_CONSTANT_COLOR)
              .addInt("GL_CONSTANT_ALPHA", GL_CONSTANT_ALPHA)
              .addInt("GL_ONE_MINUS_CONSTANT_ALPHA", GL_ONE_MINUS_CONSTANT_ALPHA);
#endif
#ifdef GL_SRC_ALPHA_SATURATE
        module.addInt("GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE);
#endif

        // Buffer copy constants
#ifdef GL_COPY_READ_BUFFER
        module.addInt("GL_COPY_READ_BUFFER", GL_COPY_READ_BUFFER)
              .addInt("GL_COPY_WRITE_BUFFER", GL_COPY_WRITE_BUFFER);
#endif

        // Map buffer range access bits
#ifdef GL_MAP_READ_BIT
        module.addInt("GL_MAP_READ_BIT", GL_MAP_READ_BIT)
              .addInt("GL_MAP_WRITE_BIT", GL_MAP_WRITE_BIT)
              .addInt("GL_MAP_INVALIDATE_RANGE_BIT", GL_MAP_INVALIDATE_RANGE_BIT)
              .addInt("GL_MAP_INVALIDATE_BUFFER_BIT", GL_MAP_INVALIDATE_BUFFER_BIT)
              .addInt("GL_MAP_FLUSH_EXPLICIT_BIT", GL_MAP_FLUSH_EXPLICIT_BIT)
              .addInt("GL_MAP_UNSYNCHRONIZED_BIT", GL_MAP_UNSYNCHRONIZED_BIT);
#endif
#ifdef GL_MAP_PERSISTENT_BIT
        module.addInt("GL_MAP_PERSISTENT_BIT", GL_MAP_PERSISTENT_BIT)
              .addInt("GL_MAP_COHERENT_BIT", GL_MAP_COHERENT_BIT);
#endif

        // Buffer texture constants
#ifdef GL_TEXTURE_BUFFER
        module.addInt("GL_TEXTURE_BUFFER", GL_TEXTURE_BUFFER);
#endif

        // Timer query constants
#ifdef GL_TIME_ELAPSED
        module.addInt("GL_TIME_ELAPSED", GL_TIME_ELAPSED);
#endif
#ifdef GL_TIMESTAMP
        module.addInt("GL_TIMESTAMP", GL_TIMESTAMP);
#endif

        // Clear buffer targets
#ifdef GL_COLOR
        module.addInt("GL_COLOR", GL_COLOR);
#endif
#ifdef GL_DEPTH
        module.addInt("GL_DEPTH", GL_DEPTH);
#endif
#ifdef GL_STENCIL
        module.addInt("GL_STENCIL", GL_STENCIL);
#endif
    }
}
