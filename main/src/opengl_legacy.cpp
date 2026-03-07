#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <cstdint>
#include <vector>

namespace SDLBindings
{
    static bool resolveClientPointerArg(const Value &value, const char *fnName, const GLvoid **outPtr)
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
        {
            return true;
        }

        if (value.isNumber())
        {
            *outPtr = (const GLvoid *)(uintptr_t)value.asNumber();
            return true;
        }

        Error("%s expects pointer, buffer, typedarray or byte offset", fnName);
        return false;
    }

    static bool resolveFloatVectorArg(const Value &value,
                                      int requiredCount,
                                      const char *fnName,
                                      const GLfloat **outPtr,
                                      std::vector<GLfloat> &scratch)
    {
        if (value.isPointer())
        {
            *outPtr = (const GLfloat *)value.asPointer();
            return true;
        }

        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            if (!buf)
            {
                Error("%s received invalid buffer", fnName);
                return false;
            }

            if (requiredCount > 0)
            {
                const size_t needed = (size_t)requiredCount * sizeof(GLfloat);
                const size_t have = (size_t)buf->count * (size_t)buf->elementSize;
                if (have < needed)
                {
                    Error("%s buffer too small", fnName);
                    return false;
                }
            }

            *outPtr = (const GLfloat *)buf->data;
            return true;
        }

        const void *typedPtr = nullptr;
        if (getBuiltinTypedArrayData(value, &typedPtr))
        {
            *outPtr = (const GLfloat *)typedPtr;
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
            if (requiredCount > 0 && count < requiredCount)
            {
                Error("%s array has too few elements", fnName);
                return false;
            }

            const int n = (requiredCount > 0) ? requiredCount : count;
            if (n <= 0)
            {
                Error("%s expects at least 1 value", fnName);
                return false;
            }

            scratch.resize((size_t)n);
            for (int i = 0; i < n; i++)
            {
                if (!arr->values[i].isNumber())
                {
                    Error("%s array must contain only numeric values", fnName);
                    return false;
                }
                scratch[(size_t)i] = (GLfloat)arr->values[i].asNumber();
            }

            *outPtr = scratch.data();
            return true;
        }

        Error("%s expects pointer, buffer, typedarray or numeric array", fnName);
        return false;
    }

    static bool resolveDoubleVectorArg(const Value &value,
                                       int requiredCount,
                                       const char *fnName,
                                       const GLdouble **outPtr,
                                       std::vector<GLdouble> &scratch)
    {
        if (value.isPointer())
        {
            *outPtr = (const GLdouble *)value.asPointer();
            return true;
        }

        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            if (!buf)
            {
                Error("%s received invalid buffer", fnName);
                return false;
            }

            if (requiredCount > 0)
            {
                const size_t needed = (size_t)requiredCount * sizeof(GLdouble);
                const size_t have = (size_t)buf->count * (size_t)buf->elementSize;
                if (have < needed)
                {
                    Error("%s buffer too small", fnName);
                    return false;
                }
            }

            *outPtr = (const GLdouble *)buf->data;
            return true;
        }

        const void *typedPtr = nullptr;
        if (getBuiltinTypedArrayData(value, &typedPtr))
        {
            *outPtr = (const GLdouble *)typedPtr;
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
            if (requiredCount > 0 && count < requiredCount)
            {
                Error("%s array has too few elements", fnName);
                return false;
            }

            const int n = (requiredCount > 0) ? requiredCount : count;
            if (n <= 0)
            {
                Error("%s expects at least 1 value", fnName);
                return false;
            }

            scratch.resize((size_t)n);
            for (int i = 0; i < n; i++)
            {
                if (!arr->values[i].isNumber())
                {
                    Error("%s array must contain only numeric values", fnName);
                    return false;
                }
                scratch[(size_t)i] = (GLdouble)arr->values[i].asNumber();
            }

            *outPtr = scratch.data();
            return true;
        }

        Error("%s expects pointer, buffer, typedarray or numeric array", fnName);
        return false;
    }

    static int lightParamCount(GLenum pname)
    {
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            return 4;
        case GL_SPOT_DIRECTION:
            return 3;
        default:
            return 1;
        }
    }

    static int materialParamCount(GLenum pname)
    {
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_AMBIENT_AND_DIFFUSE:
            return 4;
        case GL_SHININESS:
            return 1;
        default:
            return 1;
        }
    }

    static int fogParamCount(GLenum pname)
    {
        switch (pname)
        {
        case GL_FOG_COLOR:
            return 4;
        default:
            return 1;
        }
    }

    static int lightModelParamCount(GLenum pname)
    {
        switch (pname)
        {
        case GL_LIGHT_MODEL_AMBIENT:
            return 4;
        default:
            return 1;
        }
    }

    int native_glEnableClientState(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glEnableClientState expects 1 argument");
            return 0;
        }

        glEnableClientState((GLenum)args[0].asNumber());
        return 0;
    }

    int native_glDisableClientState(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDisableClientState expects 1 argument");
            return 0;
        }

        glDisableClientState((GLenum)args[0].asNumber());
        return 0;
    }

    int native_glVertexPointer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4 && argc != 5)
        {
            Error("glVertexPointer expects 4 or 5 arguments: size, type, stride, pointerOrBufferOrOffset [, byteOffset]");
            return 0;
        }

        const GLint size = (GLint)args[0].asNumber();
        const GLenum type = (GLenum)args[1].asNumber();
        const GLsizei stride = (GLsizei)args[2].asNumber();

        const GLvoid *pointer = nullptr;
        if (!resolveClientPointerArg(args[3], "glVertexPointer", &pointer))
            return 0;

        if (argc == 5)
        {
            if (!args[4].isNumber())
            {
                Error("glVertexPointer argument 5 (byteOffset) must be a number");
                return 0;
            }

            pointer = (const GLvoid *)((uintptr_t)pointer + (uintptr_t)args[4].asNumber());
        }

        glVertexPointer(size, type, stride, pointer);
        return 0;
    }

    int native_glTexCoordPointer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4 && argc != 5)
        {
            Error("glTexCoordPointer expects 4 or 5 arguments: size, type, stride, pointerOrBufferOrOffset [, byteOffset]");
            return 0;
        }

        const GLint size = (GLint)args[0].asNumber();
        const GLenum type = (GLenum)args[1].asNumber();
        const GLsizei stride = (GLsizei)args[2].asNumber();

        const GLvoid *pointer = nullptr;
        if (!resolveClientPointerArg(args[3], "glTexCoordPointer", &pointer))
            return 0;

        if (argc == 5)
        {
            if (!args[4].isNumber())
            {
                Error("glTexCoordPointer argument 5 (byteOffset) must be a number");
                return 0;
            }

            pointer = (const GLvoid *)((uintptr_t)pointer + (uintptr_t)args[4].asNumber());
        }

        glTexCoordPointer(size, type, stride, pointer);
        return 0;
    }

    int native_glColorPointer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4 && argc != 5)
        {
            Error("glColorPointer expects 4 or 5 arguments: size, type, stride, pointerOrBufferOrOffset [, byteOffset]");
            return 0;
        }

        const GLint size = (GLint)args[0].asNumber();
        const GLenum type = (GLenum)args[1].asNumber();
        const GLsizei stride = (GLsizei)args[2].asNumber();

        const GLvoid *pointer = nullptr;
        if (!resolveClientPointerArg(args[3], "glColorPointer", &pointer))
            return 0;

        if (argc == 5)
        {
            if (!args[4].isNumber())
            {
                Error("glColorPointer argument 5 (byteOffset) must be a number");
                return 0;
            }

            pointer = (const GLvoid *)((uintptr_t)pointer + (uintptr_t)args[4].asNumber());
        }

        glColorPointer(size, type, stride, pointer);
        return 0;
    }

    int native_glNormalPointer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3 && argc != 4)
        {
            Error("glNormalPointer expects 3 or 4 arguments: type, stride, pointerOrBufferOrOffset [, byteOffset]");
            return 0;
        }

        const GLenum type = (GLenum)args[0].asNumber();
        const GLsizei stride = (GLsizei)args[1].asNumber();

        const GLvoid *pointer = nullptr;
        if (!resolveClientPointerArg(args[2], "glNormalPointer", &pointer))
            return 0;

        if (argc == 4)
        {
            if (!args[3].isNumber())
            {
                Error("glNormalPointer argument 4 (byteOffset) must be a number");
                return 0;
            }

            pointer = (const GLvoid *)((uintptr_t)pointer + (uintptr_t)args[3].asNumber());
        }

        glNormalPointer(type, stride, pointer);
        return 0;
    }

    int native_glDrawArrays(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glDrawArrays expects 3 arguments: mode, first, count");
            return 0;
        }

        glDrawArrays((GLenum)args[0].asNumber(), (GLint)args[1].asNumber(), (GLsizei)args[2].asNumber());
        return 0;
    }

    int native_glDrawElementsLegacy(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4 && argc != 5)
        {
            Error("glDrawElements expects 4 or 5 arguments: mode, count, type, pointerOrBufferOrOffset [, byteOffset]");
            return 0;
        }

        const GLenum mode = (GLenum)args[0].asNumber();
        const GLsizei count = (GLsizei)args[1].asNumber();
        const GLenum type = (GLenum)args[2].asNumber();

        const GLvoid *pointer = nullptr;
        if (!resolveClientPointerArg(args[3], "glDrawElements", &pointer))
            return 0;

        if (argc == 5)
        {
            if (!args[4].isNumber())
            {
                Error("glDrawElements argument 5 (byteOffset) must be a number");
                return 0;
            }

            pointer = (const GLvoid *)((uintptr_t)pointer + (uintptr_t)args[4].asNumber());
        }

        glDrawElements(mode, count, type, pointer);
        return 0;
    }

    int native_glShadeModel(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glShadeModel expects 1 argument");
            return 0;
        }

        glShadeModel((GLenum)args[0].asNumber());
        return 0;
    }

    int native_glLightf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glLightf expects 3 arguments: light, pname, value");
            return 0;
        }

        glLightf((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLfloat)args[2].asNumber());
        return 0;
    }

    int native_glLighti(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glLighti expects 3 arguments: light, pname, value");
            return 0;
        }

        glLighti((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLint)args[2].asNumber());
        return 0;
    }

    int native_glLightfv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glLightfv expects 3 arguments: light, pname, params");
            return 0;
        }

        const GLenum light = (GLenum)args[0].asNumber();
        const GLenum pname = (GLenum)args[1].asNumber();
        const int n = lightParamCount(pname);

        const GLfloat *params = nullptr;
        std::vector<GLfloat> scratch;
        if (!resolveFloatVectorArg(args[2], n, "glLightfv", &params, scratch))
            return 0;

        glLightfv(light, pname, params);
        return 0;
    }

    int native_glLightModelf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glLightModelf expects 2 arguments: pname, value");
            return 0;
        }

        glLightModelf((GLenum)args[0].asNumber(), (GLfloat)args[1].asNumber());
        return 0;
    }

    int native_glLightModeli(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glLightModeli expects 2 arguments: pname, value");
            return 0;
        }

        glLightModeli((GLenum)args[0].asNumber(), (GLint)args[1].asNumber());
        return 0;
    }

    int native_glLightModelfv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glLightModelfv expects 2 arguments: pname, params");
            return 0;
        }

        const GLenum pname = (GLenum)args[0].asNumber();
        const int n = lightModelParamCount(pname);

        const GLfloat *params = nullptr;
        std::vector<GLfloat> scratch;
        if (!resolveFloatVectorArg(args[1], n, "glLightModelfv", &params, scratch))
            return 0;

        glLightModelfv(pname, params);
        return 0;
    }

    int native_glMaterialf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glMaterialf expects 3 arguments: face, pname, value");
            return 0;
        }

        glMaterialf((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLfloat)args[2].asNumber());
        return 0;
    }

    int native_glMateriali(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glMateriali expects 3 arguments: face, pname, value");
            return 0;
        }

        glMateriali((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLint)args[2].asNumber());
        return 0;
    }

    int native_glMaterialfv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glMaterialfv expects 3 arguments: face, pname, params");
            return 0;
        }

        const GLenum face = (GLenum)args[0].asNumber();
        const GLenum pname = (GLenum)args[1].asNumber();
        const int n = materialParamCount(pname);

        const GLfloat *params = nullptr;
        std::vector<GLfloat> scratch;
        if (!resolveFloatVectorArg(args[2], n, "glMaterialfv", &params, scratch))
            return 0;

        glMaterialfv(face, pname, params);
        return 0;
    }

    int native_glColorMaterial(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glColorMaterial expects 2 arguments: face, mode");
            return 0;
        }

        glColorMaterial((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber());
        return 0;
    }

    int native_glFogf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glFogf expects 2 arguments: pname, value");
            return 0;
        }

        glFogf((GLenum)args[0].asNumber(), (GLfloat)args[1].asNumber());
        return 0;
    }

    int native_glFogi(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glFogi expects 2 arguments: pname, value");
            return 0;
        }

        glFogi((GLenum)args[0].asNumber(), (GLint)args[1].asNumber());
        return 0;
    }

    int native_glFogfv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glFogfv expects 2 arguments: pname, params");
            return 0;
        }

        const GLenum pname = (GLenum)args[0].asNumber();
        const int n = fogParamCount(pname);

        const GLfloat *params = nullptr;
        std::vector<GLfloat> scratch;
        if (!resolveFloatVectorArg(args[1], n, "glFogfv", &params, scratch))
            return 0;

        glFogfv(pname, params);
        return 0;
    }

    int native_glClipPlane(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glClipPlane expects 2 arguments: plane, equation");
            return 0;
        }

        const GLenum plane = (GLenum)args[0].asNumber();

        const GLdouble *equation = nullptr;
        std::vector<GLdouble> scratch;
        if (!resolveDoubleVectorArg(args[1], 4, "glClipPlane", &equation, scratch))
            return 0;

        glClipPlane(plane, equation);
        return 0;
    }

    int native_glGetClipPlane(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetClipPlane expects 1 argument: plane");
            return 0;
        }

        const GLenum plane = (GLenum)args[0].asNumber();
        GLdouble equation[4] = {0.0, 0.0, 0.0, 0.0};
        glGetClipPlane(plane, equation);

        vm->pushDouble(equation[0]);
        vm->pushDouble(equation[1]);
        vm->pushDouble(equation[2]);
        vm->pushDouble(equation[3]);
        return 4;
    }

    int native_glClearDepth(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glClearDepth expects 1 argument: depth");
            return 0;
        }
        glClearDepth((GLclampd)args[0].asNumber());
        return 0;
    }

    int native_glDepthRange(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glDepthRange expects 2 arguments: near, far");
            return 0;
        }
        glDepthRange((GLclampd)args[0].asNumber(), (GLclampd)args[1].asNumber());
        return 0;
    }

    int native_glClearAccum(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glClearAccum expects 4 arguments: r, g, b, a");
            return 0;
        }
        glClearAccum((GLfloat)args[0].asNumber(),
                     (GLfloat)args[1].asNumber(),
                     (GLfloat)args[2].asNumber(),
                     (GLfloat)args[3].asNumber());
        return 0;
    }

    int native_glAccum(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glAccum expects 2 arguments: op, value");
            return 0;
        }
        glAccum((GLenum)args[0].asNumber(), (GLfloat)args[1].asNumber());
        return 0;
    }

    int native_glFrustum(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 6)
        {
            Error("glFrustum expects 6 arguments");
            return 0;
        }
        glFrustum((GLdouble)args[0].asNumber(),
                  (GLdouble)args[1].asNumber(),
                  (GLdouble)args[2].asNumber(),
                  (GLdouble)args[3].asNumber(),
                  (GLdouble)args[4].asNumber(),
                  (GLdouble)args[5].asNumber());
        return 0;
    }

    int native_glLoadMatrixf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glLoadMatrixf expects 1 argument: matrix16");
            return 0;
        }

        const GLfloat *m = nullptr;
        std::vector<GLfloat> scratch;
        if (!resolveFloatVectorArg(args[0], 16, "glLoadMatrixf", &m, scratch))
            return 0;
        glLoadMatrixf(m);
        return 0;
    }

    int native_glLoadMatrixd(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glLoadMatrixd expects 1 argument: matrix16");
            return 0;
        }

        const GLdouble *m = nullptr;
        std::vector<GLdouble> scratch;
        if (!resolveDoubleVectorArg(args[0], 16, "glLoadMatrixd", &m, scratch))
            return 0;
        glLoadMatrixd(m);
        return 0;
    }

    int native_glMultMatrixf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glMultMatrixf expects 1 argument: matrix16");
            return 0;
        }

        const GLfloat *m = nullptr;
        std::vector<GLfloat> scratch;
        if (!resolveFloatVectorArg(args[0], 16, "glMultMatrixf", &m, scratch))
            return 0;
        glMultMatrixf(m);
        return 0;
    }

    int native_glMultMatrixd(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glMultMatrixd expects 1 argument: matrix16");
            return 0;
        }

        const GLdouble *m = nullptr;
        std::vector<GLdouble> scratch;
        if (!resolveDoubleVectorArg(args[0], 16, "glMultMatrixd", &m, scratch))
            return 0;
        glMultMatrixd(m);
        return 0;
    }

    int native_glGenLists(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGenLists expects 1 argument: range");
            return 0;
        }

        const GLuint id = glGenLists((GLsizei)args[0].asNumber());
        vm->pushInt((int)id);
        return 1;
    }

    int native_glNewList(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glNewList expects 2 arguments: list, mode");
            return 0;
        }
        glNewList((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber());
        return 0;
    }

    int native_glEndList(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("glEndList expects 0 arguments");
            return 0;
        }
        glEndList();
        return 0;
    }

    int native_glCallList(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glCallList expects 1 argument: list");
            return 0;
        }
        glCallList((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glCallLists(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glCallLists expects 3 arguments: n, type, lists");
            return 0;
        }

        const GLsizei n = (GLsizei)args[0].asNumber();
        const GLenum type = (GLenum)args[1].asNumber();

        const GLvoid *lists = nullptr;
        if (!resolveClientPointerArg(args[2], "glCallLists", &lists))
            return 0;

        glCallLists(n, type, lists);
        return 0;
    }

    int native_glDeleteLists(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glDeleteLists expects 2 arguments: list, range");
            return 0;
        }
        glDeleteLists((GLuint)args[0].asNumber(), (GLsizei)args[1].asNumber());
        return 0;
    }

    int native_glListBase(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glListBase expects 1 argument: base");
            return 0;
        }
        glListBase((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glRectf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glRectf expects 4 arguments: x1, y1, x2, y2");
            return 0;
        }
        glRectf((GLfloat)args[0].asNumber(),
                (GLfloat)args[1].asNumber(),
                (GLfloat)args[2].asNumber(),
                (GLfloat)args[3].asNumber());
        return 0;
    }

    int native_glRectd(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glRectd expects 4 arguments: x1, y1, x2, y2");
            return 0;
        }
        glRectd((GLdouble)args[0].asNumber(),
                (GLdouble)args[1].asNumber(),
                (GLdouble)args[2].asNumber(),
                (GLdouble)args[3].asNumber());
        return 0;
    }

    int native_glRasterPos2f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glRasterPos2f expects 2 arguments");
            return 0;
        }
        glRasterPos2f((GLfloat)args[0].asNumber(), (GLfloat)args[1].asNumber());
        return 0;
    }

    int native_glRasterPos3f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glRasterPos3f expects 3 arguments");
            return 0;
        }
        glRasterPos3f((GLfloat)args[0].asNumber(),
                      (GLfloat)args[1].asNumber(),
                      (GLfloat)args[2].asNumber());
        return 0;
    }

    int native_glRasterPos4f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glRasterPos4f expects 4 arguments");
            return 0;
        }
        glRasterPos4f((GLfloat)args[0].asNumber(),
                      (GLfloat)args[1].asNumber(),
                      (GLfloat)args[2].asNumber(),
                      (GLfloat)args[3].asNumber());
        return 0;
    }

    int native_glBitmap(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 7)
        {
            Error("glBitmap expects 7 arguments: width, height, xorig, yorig, xmove, ymove, bitmap");
            return 0;
        }

        const GLvoid *bitmap = nullptr;
        if (!resolveClientPointerArg(args[6], "glBitmap", &bitmap))
            return 0;

        glBitmap((GLsizei)args[0].asNumber(),
                 (GLsizei)args[1].asNumber(),
                 (GLfloat)args[2].asNumber(),
                 (GLfloat)args[3].asNumber(),
                 (GLfloat)args[4].asNumber(),
                 (GLfloat)args[5].asNumber(),
                 (const GLubyte *)bitmap);
        return 0;
    }

    int native_glDrawPixels(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glDrawPixels expects 5 arguments: width, height, format, type, pixels");
            return 0;
        }

        const GLvoid *pixels = nullptr;
        if (!resolveClientPointerArg(args[4], "glDrawPixels", &pixels))
            return 0;

        glDrawPixels((GLsizei)args[0].asNumber(),
                     (GLsizei)args[1].asNumber(),
                     (GLenum)args[2].asNumber(),
                     (GLenum)args[3].asNumber(),
                     pixels);
        return 0;
    }

    int native_glReadPixels(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 7)
        {
            Error("glReadPixels expects 7 arguments: x, y, width, height, format, type, outPixels");
            return 0;
        }

        const GLvoid *outPixels = nullptr;
        if (!resolveClientPointerArg(args[6], "glReadPixels", &outPixels))
            return 0;

        glReadPixels((GLint)args[0].asNumber(),
                     (GLint)args[1].asNumber(),
                     (GLsizei)args[2].asNumber(),
                     (GLsizei)args[3].asNumber(),
                     (GLenum)args[4].asNumber(),
                     (GLenum)args[5].asNumber(),
                     (GLvoid *)outPixels);
        return 0;
    }

    int native_glClearStencil(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glClearStencil expects 1 argument: s");
            return 0;
        }
        glClearStencil((GLint)args[0].asNumber());
        return 0;
    }

    int native_glStencilFunc(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glStencilFunc expects 3 arguments: func, ref, mask");
            return 0;
        }
        glStencilFunc((GLenum)args[0].asNumber(), (GLint)args[1].asNumber(), (GLuint)args[2].asNumber());
        return 0;
    }

    int native_glStencilMask(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glStencilMask expects 1 argument: mask");
            return 0;
        }
        glStencilMask((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glStencilOp(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glStencilOp expects 3 arguments: fail, zfail, zpass");
            return 0;
        }
        glStencilOp((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLenum)args[2].asNumber());
        return 0;
    }

    int native_glMap1f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 6)
        {
            Error("glMap1f expects 6 arguments: target, u1, u2, stride, order, points");
            return 0;
        }

        const GLvoid *points = nullptr;
        if (!resolveClientPointerArg(args[5], "glMap1f", &points))
            return 0;

        glMap1f((GLenum)args[0].asNumber(),
                (GLfloat)args[1].asNumber(),
                (GLfloat)args[2].asNumber(),
                (GLint)args[3].asNumber(),
                (GLint)args[4].asNumber(),
                (const GLfloat *)points);
        return 0;
    }

    int native_glMap1d(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 6)
        {
            Error("glMap1d expects 6 arguments: target, u1, u2, stride, order, points");
            return 0;
        }

        const GLvoid *points = nullptr;
        if (!resolveClientPointerArg(args[5], "glMap1d", &points))
            return 0;

        glMap1d((GLenum)args[0].asNumber(),
                (GLdouble)args[1].asNumber(),
                (GLdouble)args[2].asNumber(),
                (GLint)args[3].asNumber(),
                (GLint)args[4].asNumber(),
                (const GLdouble *)points);
        return 0;
    }

    int native_glMap2f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 10)
        {
            Error("glMap2f expects 10 arguments: target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points");
            return 0;
        }

        const GLvoid *points = nullptr;
        if (!resolveClientPointerArg(args[9], "glMap2f", &points))
            return 0;

        glMap2f((GLenum)args[0].asNumber(),
                (GLfloat)args[1].asNumber(),
                (GLfloat)args[2].asNumber(),
                (GLint)args[3].asNumber(),
                (GLint)args[4].asNumber(),
                (GLfloat)args[5].asNumber(),
                (GLfloat)args[6].asNumber(),
                (GLint)args[7].asNumber(),
                (GLint)args[8].asNumber(),
                (const GLfloat *)points);
        return 0;
    }

    int native_glMap2d(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 10)
        {
            Error("glMap2d expects 10 arguments: target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points");
            return 0;
        }

        const GLvoid *points = nullptr;
        if (!resolveClientPointerArg(args[9], "glMap2d", &points))
            return 0;

        glMap2d((GLenum)args[0].asNumber(),
                (GLdouble)args[1].asNumber(),
                (GLdouble)args[2].asNumber(),
                (GLint)args[3].asNumber(),
                (GLint)args[4].asNumber(),
                (GLdouble)args[5].asNumber(),
                (GLdouble)args[6].asNumber(),
                (GLint)args[7].asNumber(),
                (GLint)args[8].asNumber(),
                (const GLdouble *)points);
        return 0;
    }

    int native_glMapGrid1f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glMapGrid1f expects 3 arguments: un, u1, u2");
            return 0;
        }
        glMapGrid1f((GLint)args[0].asNumber(), (GLfloat)args[1].asNumber(), (GLfloat)args[2].asNumber());
        return 0;
    }

    int native_glMapGrid2f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 6)
        {
            Error("glMapGrid2f expects 6 arguments: un, u1, u2, vn, v1, v2");
            return 0;
        }
        glMapGrid2f((GLint)args[0].asNumber(),
                    (GLfloat)args[1].asNumber(),
                    (GLfloat)args[2].asNumber(),
                    (GLint)args[3].asNumber(),
                    (GLfloat)args[4].asNumber(),
                    (GLfloat)args[5].asNumber());
        return 0;
    }

    int native_glEvalCoord1f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glEvalCoord1f expects 1 argument: u");
            return 0;
        }
        glEvalCoord1f((GLfloat)args[0].asNumber());
        return 0;
    }

    int native_glEvalCoord2f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glEvalCoord2f expects 2 arguments: u, v");
            return 0;
        }
        glEvalCoord2f((GLfloat)args[0].asNumber(), (GLfloat)args[1].asNumber());
        return 0;
    }

    int native_glEvalPoint1(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glEvalPoint1 expects 1 argument: i");
            return 0;
        }
        glEvalPoint1((GLint)args[0].asNumber());
        return 0;
    }

    int native_glEvalPoint2(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glEvalPoint2 expects 2 arguments: i, j");
            return 0;
        }
        glEvalPoint2((GLint)args[0].asNumber(), (GLint)args[1].asNumber());
        return 0;
    }

    int native_glEvalMesh1(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glEvalMesh1 expects 3 arguments: mode, i1, i2");
            return 0;
        }
        glEvalMesh1((GLenum)args[0].asNumber(), (GLint)args[1].asNumber(), (GLint)args[2].asNumber());
        return 0;
    }

    int native_glEvalMesh2(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glEvalMesh2 expects 5 arguments: mode, i1, i2, j1, j2");
            return 0;
        }
        glEvalMesh2((GLenum)args[0].asNumber(),
                    (GLint)args[1].asNumber(),
                    (GLint)args[2].asNumber(),
                    (GLint)args[3].asNumber(),
                    (GLint)args[4].asNumber());
        return 0;
    }

    void register_opengl_legacy(ModuleBuilder &module)
    {
        module.addFunction("glEnableClientState", native_glEnableClientState, 1)
            .addFunction("glDisableClientState", native_glDisableClientState, 1)
            .addFunction("glVertexPointer", native_glVertexPointer, -1)
            .addFunction("glTexCoordPointer", native_glTexCoordPointer, -1)
            .addFunction("glColorPointer", native_glColorPointer, -1)
            .addFunction("glNormalPointer", native_glNormalPointer, -1)
            .addFunction("glDrawArrays", native_glDrawArrays, 3)
            .addFunction("glDrawElementsLegacy", native_glDrawElementsLegacy, -1)
            .addFunction("glShadeModel", native_glShadeModel, 1)
            .addFunction("glLightf", native_glLightf, 3)
            .addFunction("glLighti", native_glLighti, 3)
            .addFunction("glLightfv", native_glLightfv, 3)
            .addFunction("glLightModelf", native_glLightModelf, 2)
            .addFunction("glLightModeli", native_glLightModeli, 2)
            .addFunction("glLightModelfv", native_glLightModelfv, 2)
            .addFunction("glMaterialf", native_glMaterialf, 3)
            .addFunction("glMateriali", native_glMateriali, 3)
            .addFunction("glMaterialfv", native_glMaterialfv, 3)
            .addFunction("glColorMaterial", native_glColorMaterial, 2)
            .addFunction("glFogf", native_glFogf, 2)
            .addFunction("glFogi", native_glFogi, 2)
            .addFunction("glFogfv", native_glFogfv, 2)
            .addFunction("glClipPlane", native_glClipPlane, 2)
            .addFunction("glGetClipPlane", native_glGetClipPlane, 1)
            .addFunction("glClearDepth", native_glClearDepth, 1)
            .addFunction("glDepthRange", native_glDepthRange, 2)
            .addFunction("glClearAccum", native_glClearAccum, 4)
            .addFunction("glAccum", native_glAccum, 2)
            .addFunction("glFrustum", native_glFrustum, 6)
            .addFunction("glLoadMatrixf", native_glLoadMatrixf, 1)
            .addFunction("glLoadMatrixd", native_glLoadMatrixd, 1)
            .addFunction("glMultMatrixf", native_glMultMatrixf, 1)
            .addFunction("glMultMatrixd", native_glMultMatrixd, 1)
            .addFunction("glGenLists", native_glGenLists, 1)
            .addFunction("glNewList", native_glNewList, 2)
            .addFunction("glEndList", native_glEndList, 0)
            .addFunction("glCallList", native_glCallList, 1)
            .addFunction("glCallLists", native_glCallLists, 3)
            .addFunction("glDeleteLists", native_glDeleteLists, 2)
            .addFunction("glListBase", native_glListBase, 1)
            .addFunction("glRectf", native_glRectf, 4)
            .addFunction("glRectd", native_glRectd, 4)
            .addFunction("glRasterPos2f", native_glRasterPos2f, 2)
            .addFunction("glRasterPos3f", native_glRasterPos3f, 3)
            .addFunction("glRasterPos4f", native_glRasterPos4f, 4)
            .addFunction("glBitmap", native_glBitmap, 7)
            .addFunction("glDrawPixels", native_glDrawPixels, 5)
            .addFunction("glReadPixels", native_glReadPixels, 7)
            .addFunction("glClearStencil", native_glClearStencil, 1)
            .addFunction("glStencilFunc", native_glStencilFunc, 3)
            .addFunction("glStencilMask", native_glStencilMask, 1)
            .addFunction("glStencilOp", native_glStencilOp, 3)
            .addFunction("glMap1f", native_glMap1f, 6)
            .addFunction("glMap1d", native_glMap1d, 6)
            .addFunction("glMap2f", native_glMap2f, 10)
            .addFunction("glMap2d", native_glMap2d, 10)
            .addFunction("glMapGrid1f", native_glMapGrid1f, 3)
            .addFunction("glMapGrid2f", native_glMapGrid2f, 6)
            .addFunction("glEvalCoord1f", native_glEvalCoord1f, 1)
            .addFunction("glEvalCoord2f", native_glEvalCoord2f, 2)
            .addFunction("glEvalPoint1", native_glEvalPoint1, 1)
            .addFunction("glEvalPoint2", native_glEvalPoint2, 2)
            .addFunction("glEvalMesh1", native_glEvalMesh1, 3)
            .addFunction("glEvalMesh2", native_glEvalMesh2, 5)

            .addInt("GL_VERTEX_ARRAY", GL_VERTEX_ARRAY)
            .addInt("GL_NORMAL_ARRAY", GL_NORMAL_ARRAY)
            .addInt("GL_COLOR_ARRAY", GL_COLOR_ARRAY)
            .addInt("GL_TEXTURE_COORD_ARRAY", GL_TEXTURE_COORD_ARRAY)
            .addInt("GL_LIGHTING", GL_LIGHTING)
            .addInt("GL_LIGHT0", GL_LIGHT0)
            .addInt("GL_LIGHT1", GL_LIGHT1)
            .addInt("GL_LIGHT2", GL_LIGHT2)
            .addInt("GL_LIGHT3", GL_LIGHT3)
            .addInt("GL_LIGHT4", GL_LIGHT4)
            .addInt("GL_LIGHT5", GL_LIGHT5)
            .addInt("GL_LIGHT6", GL_LIGHT6)
            .addInt("GL_LIGHT7", GL_LIGHT7)
            .addInt("GL_POSITION", GL_POSITION)
            .addInt("GL_AMBIENT", GL_AMBIENT)
            .addInt("GL_DIFFUSE", GL_DIFFUSE)
            .addInt("GL_SPECULAR", GL_SPECULAR)
            .addInt("GL_SPOT_DIRECTION", GL_SPOT_DIRECTION)
            .addInt("GL_SPOT_EXPONENT", GL_SPOT_EXPONENT)
            .addInt("GL_SPOT_CUTOFF", GL_SPOT_CUTOFF)
            .addInt("GL_CONSTANT_ATTENUATION", GL_CONSTANT_ATTENUATION)
            .addInt("GL_LINEAR_ATTENUATION", GL_LINEAR_ATTENUATION)
            .addInt("GL_QUADRATIC_ATTENUATION", GL_QUADRATIC_ATTENUATION)
            .addInt("GL_LIGHT_MODEL_AMBIENT", GL_LIGHT_MODEL_AMBIENT)
            .addInt("GL_LIGHT_MODEL_LOCAL_VIEWER", GL_LIGHT_MODEL_LOCAL_VIEWER)
            .addInt("GL_LIGHT_MODEL_TWO_SIDE", GL_LIGHT_MODEL_TWO_SIDE)
            .addInt("GL_AMBIENT_AND_DIFFUSE", GL_AMBIENT_AND_DIFFUSE)
            .addInt("GL_EMISSION", GL_EMISSION)
            .addInt("GL_SHININESS", GL_SHININESS)
            .addInt("GL_COLOR_MATERIAL", GL_COLOR_MATERIAL)
            .addInt("GL_FOG", GL_FOG)
            .addInt("GL_FOG_MODE", GL_FOG_MODE)
            .addInt("GL_FOG_DENSITY", GL_FOG_DENSITY)
            .addInt("GL_FOG_START", GL_FOG_START)
            .addInt("GL_FOG_END", GL_FOG_END)
            .addInt("GL_FOG_COLOR", GL_FOG_COLOR)
            .addInt("GL_EXP", GL_EXP)
            .addInt("GL_EXP2", GL_EXP2)
            .addInt("GL_FLAT", GL_FLAT)
            .addInt("GL_SMOOTH", GL_SMOOTH);

#ifdef GL_ACCUM_BUFFER_BIT
        module.addInt("GL_ACCUM_BUFFER_BIT", GL_ACCUM_BUFFER_BIT);
#endif
#ifdef GL_ACCUM
        module.addInt("GL_ACCUM", GL_ACCUM);
#endif
#ifdef GL_LOAD
        module.addInt("GL_LOAD", GL_LOAD);
#endif
#ifdef GL_RETURN
        module.addInt("GL_RETURN", GL_RETURN);
#endif
#ifdef GL_MULT
        module.addInt("GL_MULT", GL_MULT);
#endif
#ifdef GL_DEPTH_COMPONENT
        module.addInt("GL_DEPTH_COMPONENT", GL_DEPTH_COMPONENT);
#endif
#ifdef GL_DEPTH_BITS
        module.addInt("GL_DEPTH_BITS", GL_DEPTH_BITS);
#endif
#ifdef GL_DEPTH_CLEAR_VALUE
        module.addInt("GL_DEPTH_CLEAR_VALUE", GL_DEPTH_CLEAR_VALUE);
#endif
#ifdef GL_COMPILE
        module.addInt("GL_COMPILE", GL_COMPILE);
#endif
#ifdef GL_COMPILE_AND_EXECUTE
        module.addInt("GL_COMPILE_AND_EXECUTE", GL_COMPILE_AND_EXECUTE);
#endif
#ifdef GL_LIST_BASE
        module.addInt("GL_LIST_BASE", GL_LIST_BASE);
#endif
#ifdef GL_LIST_INDEX
        module.addInt("GL_LIST_INDEX", GL_LIST_INDEX);
#endif
#ifdef GL_LIST_MODE
        module.addInt("GL_LIST_MODE", GL_LIST_MODE);
#endif
#ifdef GL_STENCIL_TEST
        module.addInt("GL_STENCIL_TEST", GL_STENCIL_TEST);
#endif
#ifdef GL_STENCIL_BITS
        module.addInt("GL_STENCIL_BITS", GL_STENCIL_BITS);
#endif
#ifdef GL_STENCIL_CLEAR_VALUE
        module.addInt("GL_STENCIL_CLEAR_VALUE", GL_STENCIL_CLEAR_VALUE);
#endif
#ifdef GL_STENCIL_REF
        module.addInt("GL_STENCIL_REF", GL_STENCIL_REF);
#endif
#ifdef GL_STENCIL_VALUE_MASK
        module.addInt("GL_STENCIL_VALUE_MASK", GL_STENCIL_VALUE_MASK);
#endif
#ifdef GL_STENCIL_WRITEMASK
        module.addInt("GL_STENCIL_WRITEMASK", GL_STENCIL_WRITEMASK);
#endif
#ifdef GL_STENCIL_FAIL
        module.addInt("GL_STENCIL_FAIL", GL_STENCIL_FAIL);
#endif
#ifdef GL_STENCIL_PASS_DEPTH_FAIL
        module.addInt("GL_STENCIL_PASS_DEPTH_FAIL", GL_STENCIL_PASS_DEPTH_FAIL);
#endif
#ifdef GL_STENCIL_PASS_DEPTH_PASS
        module.addInt("GL_STENCIL_PASS_DEPTH_PASS", GL_STENCIL_PASS_DEPTH_PASS);
#endif
#ifdef GL_KEEP
        module.addInt("GL_KEEP", GL_KEEP);
#endif
#ifdef GL_INCR
        module.addInt("GL_INCR", GL_INCR);
#endif
#ifdef GL_DECR
        module.addInt("GL_DECR", GL_DECR);
#endif
#ifdef GL_CURRENT_RASTER_POSITION
        module.addInt("GL_CURRENT_RASTER_POSITION", GL_CURRENT_RASTER_POSITION);
#endif
#ifdef GL_CURRENT_RASTER_POSITION_VALID
        module.addInt("GL_CURRENT_RASTER_POSITION_VALID", GL_CURRENT_RASTER_POSITION_VALID);
#endif
#ifdef GL_CURRENT_RASTER_COLOR
        module.addInt("GL_CURRENT_RASTER_COLOR", GL_CURRENT_RASTER_COLOR);
#endif
#ifdef GL_AUTO_NORMAL
        module.addInt("GL_AUTO_NORMAL", GL_AUTO_NORMAL);
#endif
#ifdef GL_MAP1_VERTEX_3
        module.addInt("GL_MAP1_VERTEX_3", GL_MAP1_VERTEX_3);
#endif
#ifdef GL_MAP1_VERTEX_4
        module.addInt("GL_MAP1_VERTEX_4", GL_MAP1_VERTEX_4);
#endif
#ifdef GL_MAP1_INDEX
        module.addInt("GL_MAP1_INDEX", GL_MAP1_INDEX);
#endif
#ifdef GL_MAP1_COLOR_4
        module.addInt("GL_MAP1_COLOR_4", GL_MAP1_COLOR_4);
#endif
#ifdef GL_MAP1_NORMAL
        module.addInt("GL_MAP1_NORMAL", GL_MAP1_NORMAL);
#endif
#ifdef GL_MAP1_TEXTURE_COORD_1
        module.addInt("GL_MAP1_TEXTURE_COORD_1", GL_MAP1_TEXTURE_COORD_1);
#endif
#ifdef GL_MAP1_TEXTURE_COORD_2
        module.addInt("GL_MAP1_TEXTURE_COORD_2", GL_MAP1_TEXTURE_COORD_2);
#endif
#ifdef GL_MAP1_TEXTURE_COORD_3
        module.addInt("GL_MAP1_TEXTURE_COORD_3", GL_MAP1_TEXTURE_COORD_3);
#endif
#ifdef GL_MAP1_TEXTURE_COORD_4
        module.addInt("GL_MAP1_TEXTURE_COORD_4", GL_MAP1_TEXTURE_COORD_4);
#endif
#ifdef GL_MAP2_VERTEX_3
        module.addInt("GL_MAP2_VERTEX_3", GL_MAP2_VERTEX_3);
#endif
#ifdef GL_MAP2_VERTEX_4
        module.addInt("GL_MAP2_VERTEX_4", GL_MAP2_VERTEX_4);
#endif
#ifdef GL_MAP2_INDEX
        module.addInt("GL_MAP2_INDEX", GL_MAP2_INDEX);
#endif
#ifdef GL_MAP2_COLOR_4
        module.addInt("GL_MAP2_COLOR_4", GL_MAP2_COLOR_4);
#endif
#ifdef GL_MAP2_NORMAL
        module.addInt("GL_MAP2_NORMAL", GL_MAP2_NORMAL);
#endif
#ifdef GL_MAP2_TEXTURE_COORD_1
        module.addInt("GL_MAP2_TEXTURE_COORD_1", GL_MAP2_TEXTURE_COORD_1);
#endif
#ifdef GL_MAP2_TEXTURE_COORD_2
        module.addInt("GL_MAP2_TEXTURE_COORD_2", GL_MAP2_TEXTURE_COORD_2);
#endif
#ifdef GL_MAP2_TEXTURE_COORD_3
        module.addInt("GL_MAP2_TEXTURE_COORD_3", GL_MAP2_TEXTURE_COORD_3);
#endif
#ifdef GL_MAP2_TEXTURE_COORD_4
        module.addInt("GL_MAP2_TEXTURE_COORD_4", GL_MAP2_TEXTURE_COORD_4);
#endif
#ifdef GL_MAP1_GRID_DOMAIN
        module.addInt("GL_MAP1_GRID_DOMAIN", GL_MAP1_GRID_DOMAIN);
#endif
#ifdef GL_MAP1_GRID_SEGMENTS
        module.addInt("GL_MAP1_GRID_SEGMENTS", GL_MAP1_GRID_SEGMENTS);
#endif
#ifdef GL_MAP2_GRID_DOMAIN
        module.addInt("GL_MAP2_GRID_DOMAIN", GL_MAP2_GRID_DOMAIN);
#endif
#ifdef GL_MAP2_GRID_SEGMENTS
        module.addInt("GL_MAP2_GRID_SEGMENTS", GL_MAP2_GRID_SEGMENTS);
#endif
#ifdef GL_COEFF
        module.addInt("GL_COEFF", GL_COEFF);
#endif
#ifdef GL_ORDER
        module.addInt("GL_ORDER", GL_ORDER);
#endif
#ifdef GL_DOMAIN
        module.addInt("GL_DOMAIN", GL_DOMAIN);
#endif

#ifdef GL_CLIP_PLANE0
        module.addInt("GL_CLIP_PLANE0", GL_CLIP_PLANE0);
#endif
#ifdef GL_CLIP_PLANE1
        module.addInt("GL_CLIP_PLANE1", GL_CLIP_PLANE1);
#endif
#ifdef GL_CLIP_PLANE2
        module.addInt("GL_CLIP_PLANE2", GL_CLIP_PLANE2);
#endif
#ifdef GL_CLIP_PLANE3
        module.addInt("GL_CLIP_PLANE3", GL_CLIP_PLANE3);
#endif
#ifdef GL_CLIP_PLANE4
        module.addInt("GL_CLIP_PLANE4", GL_CLIP_PLANE4);
#endif
#ifdef GL_CLIP_PLANE5
        module.addInt("GL_CLIP_PLANE5", GL_CLIP_PLANE5);
#endif
#ifdef GL_MAX_CLIP_PLANES
        module.addInt("GL_MAX_CLIP_PLANES", GL_MAX_CLIP_PLANES);
#endif
    }
}
