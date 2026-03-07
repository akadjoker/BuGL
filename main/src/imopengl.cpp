#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

 
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

namespace SDLBindings
{
    static bool g_glDebugEnabled = false;

    void set_gl_debug_enabled(bool enabled)
    {
        g_glDebugEnabled = enabled;
    }

    bool get_gl_debug_enabled()
    {
        return g_glDebugEnabled;
    }

    static const char *gl_error_to_string(GLenum errorCode)
    {
        switch (errorCode)
        {
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
#ifdef GL_STACK_OVERFLOW
        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";
#endif
#ifdef GL_STACK_UNDERFLOW
        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";
#endif
        default:
            return "GL_UNKNOWN_ERROR";
        }
    }

    bool check_gl_errors(Interpreter *vm, const char *fnName)
    {
        if (!g_glDebugEnabled)
            return true;

        GLenum errorCode = glGetError();
        if (errorCode == GL_NO_ERROR)
            return true;

        const char *errorString = gl_error_to_string(errorCode);
        if (vm)
            vm->runtimeError("%s failed: %s (0x%04x)", fnName ? fnName : "OpenGL", errorString, (unsigned)errorCode);
        else
            Error("%s failed: %s (0x%04x)", fnName ? fnName : "OpenGL", errorString, (unsigned)errorCode);

        while ((errorCode = glGetError()) != GL_NO_ERROR)
        {
        }
        return false;
    }


    // =====================================================
    // BASIC OPENGL FUNCTIONS
    // =====================================================

    int native_glClearColor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glClearColor expects 4 arguments (r, g, b, a)");
            return 0;
        }

        GLfloat r = args[0].asNumber();
        GLfloat g = args[1].asNumber();
        GLfloat b = args[2].asNumber();
        GLfloat a = args[3].asNumber();
        glClearColor(r, g, b, a);
        return 0;
    }

    int native_glClear(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glClear expects 1 argument");
            return 0;
        }

        GLbitfield mask = args[0].asNumber();
        glClear(mask);
        return 0;
    }

    int native_glViewport(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glViewport expects 4 arguments (x, y, width, height)");
            return 0;
        }

        GLint x = args[0].asNumber();
        GLint y = args[1].asNumber();
        GLsizei width = args[2].asNumber();
        GLsizei height = args[3].asNumber();
        glViewport(x, y, width, height);
        return 0;
    }

    int native_glEnable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glEnable expects 1 argument");
            return 0;
        }

        GLenum cap = args[0].asNumber();
        glEnable(cap);
        return 0;
    }

    int native_glDisable(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glDisable expects 1 argument");
            return 0;
        }

        GLenum cap = args[0].asNumber();
        glDisable(cap);
        return 0;
    }

    int native_glBlendFunc(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glBlendFunc expects 2 arguments");
            return 0;
        }

        GLenum sfactor = args[0].asNumber();
        GLenum dfactor = args[1].asNumber();
        glBlendFunc(sfactor, dfactor);
        return 0;
    }

    int native_glDepthFunc(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glDepthFunc expects 1 argument");
            return 0;
        }

        GLenum func = args[0].asNumber();
        glDepthFunc(func);
        return 0;
    }

    int native_glDepthMask(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glDepthMask expects 1 argument");
            return 0;
        }

        GLboolean flag = args[0].asBool() ? GL_TRUE : GL_FALSE;
        glDepthMask(flag);
        return 0;
    }

    int native_glCullFace(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glCullFace expects 1 argument");
            return 0;
        }

        GLenum mode = args[0].asNumber();
        glCullFace(mode);
        return 0;
    }

    int native_glFrontFace(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glFrontFace expects 1 argument");
            return 0;
        }

        GLenum mode = args[0].asNumber();
        glFrontFace(mode);
        return 0;
    }

    int native_glPolygonMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glPolygonMode expects 2 arguments");
            return 0;
        }

        GLenum face = args[0].asNumber();
        GLenum mode = args[1].asNumber();
        glPolygonMode(face, mode);
        return 0;
    }

    int native_glLineWidth(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glLineWidth expects 1 argument");
            return 0;
        }

        GLfloat width = args[0].asNumber();
        glLineWidth(width);
        return 0;
    }

    int native_glPointSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glPointSize expects 1 argument");
            return 0;
        }

        GLfloat size = args[0].asNumber();
        glPointSize(size);
        return 0;
    }

    int native_glScissor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glScissor expects 4 arguments");
            return 0;
        }

        GLint x = args[0].asNumber();
        GLint y = args[1].asNumber();
        GLsizei width = args[2].asNumber();
        GLsizei height = args[3].asNumber();
        glScissor(x, y, width, height);
        return 0;
    }

    int native_glGetError(Interpreter *vm, int argc, Value *args)
    {
        GLenum error = glGetError();
        vm->pushInt(error);
        return 0;
    }

    int native_GLDebug(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("GLDebug expects 1 argument: enabled");
            return 0;
        }

        const bool enabled = args[0].asBool();
        set_gl_debug_enabled(enabled);
        vm->pushBool(enabled);
        return 1;
    }

    int native_GLCheck(Interpreter *vm, int argc, Value *args)
    {
        const char *label = "GLCheck";
        if (argc == 1)
        {
            if (!args[0].isString())
            {
                Error("GLCheck expects optional string label");
                return 0;
            }
            label = args[0].asStringChars();
        }
        else if (argc != 0)
        {
            Error("GLCheck expects 0 or 1 arguments");
            return 0;
        }

        const bool ok = check_gl_errors(vm, label);
        vm->pushBool(ok);
        return 1;
    }

    int native_glGetString(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetString expects 1 argument");
            return 0;
        }

        GLenum name = args[0].asNumber();
        const GLubyte *str = glGetString(name);
        if (str)
            vm->pushString((const char *)str);
        else 
            vm->pushNil();
        return 0;
    }

    int native_glGetIntegerv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetIntegerv expects 1 argument");
            return 0;
        }

        GLenum pname = args[0].asNumber();
        GLint value;
        glGetIntegerv(pname, &value);
        vm->pushInt(value);
        return 1;
    }

    int native_glFlush(Interpreter *vm, int argc, Value *args)
    {
        glFlush();
        return 0;
    }

    int native_glFinish(Interpreter *vm, int argc, Value *args)
    {
        glFinish();
        return 0;
    }

    // =====================================================
    // IMMEDIATE MODE (OpenGL 1.x - útil para aprender)
    // =====================================================

    int native_glBegin(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glBegin expects 1 argument");
            return 0;
        }

        GLenum mode = args[0].asNumber();
        glBegin(mode);
        return 0;
    }

    int native_glEnd(Interpreter *vm, int argc, Value *args)
    {
        glEnd();
        return 0;
    }

    int native_glVertex2f(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glVertex2f expects 2 arguments");
            return 0;
        }

        GLfloat x = args[0].asNumber();
        GLfloat y = args[1].asNumber();
        glVertex2f(x, y);
        return 0;
    }

    int native_glVertex3f(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glVertex3f expects 3 arguments");
            return 0;
        }

        GLfloat x = args[0].asNumber();
        GLfloat y = args[1].asNumber();
        GLfloat z = args[2].asNumber();
        glVertex3f(x, y, z);
        return 0;
    }

    int native_glColor3f(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glColor3f expects 3 arguments");
            return 0;
        }

        GLfloat r = args[0].asNumber();
        GLfloat g = args[1].asNumber();
        GLfloat b = args[2].asNumber();
        glColor3f(r, g, b);
        return 0;
    }

    int native_glColor4f(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glColor4f expects 4 arguments");
            return 0;
        }

        GLfloat r = args[0].asNumber();
        GLfloat g = args[1].asNumber();
        GLfloat b = args[2].asNumber();
        GLfloat a = args[3].asNumber();
        glColor4f(r, g, b, a);
        return 0;
    }

    int native_glTexCoord2f(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glTexCoord2f expects 2 arguments");
            return 0;
        }

        GLfloat s = args[0].asNumber();
        GLfloat t = args[1].asNumber();
        glTexCoord2f(s, t);
        return 0;
    }

    int native_glNormal3f(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glNormal3f expects 3 arguments");
            return 0;
        }

        GLfloat nx = args[0].asNumber();
        GLfloat ny = args[1].asNumber();
        GLfloat nz = args[2].asNumber();
        glNormal3f(nx, ny, nz);
        return 0;
    }

    // =====================================================
    // MATRIX OPERATIONS (OpenGL 1.x)
    // =====================================================

    int native_glMatrixMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glMatrixMode expects 1 argument");
            return 0;
        }

        GLenum mode = args[0].asNumber();
        glMatrixMode(mode);
        return 0;
    }

    int native_glLoadIdentity(Interpreter *vm, int argc, Value *args)
    {
        glLoadIdentity();
        return 0;
    }

    int native_glPushMatrix(Interpreter *vm, int argc, Value *args)
    {
        glPushMatrix();
        return 0;
    }

    int native_glPopMatrix(Interpreter *vm, int argc, Value *args)
    {
        glPopMatrix();
        return 0;
    }

    int native_glTranslatef(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glTranslatef expects 3 arguments");
            return 0;
        }

        GLfloat x = args[0].asNumber();
        GLfloat y = args[1].asNumber();
        GLfloat z = args[2].asNumber();
        glTranslatef(x, y, z);
        return 0;
    }

    int native_glRotatef(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glRotatef expects 4 arguments (angle, x, y, z)");
            return 0;
        }

        GLfloat angle = args[0].asNumber();
        GLfloat x = args[1].asNumber();
        GLfloat y = args[2].asNumber();
        GLfloat z = args[3].asNumber();
        glRotatef(angle, x, y, z);
        return 0;
    }

    int native_glScalef(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glScalef expects 3 arguments");
            return 0;
        }

        GLfloat x = args[0].asNumber();
        GLfloat y = args[1].asNumber();
        GLfloat z = args[2].asNumber();
        glScalef(x, y, z);
        return 0;
    }

    int native_glOrtho(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("glOrtho expects 6 arguments");
            return 0;
        }

        GLdouble left = args[0].asNumber();
        GLdouble right = args[1].asNumber();
        GLdouble bottom = args[2].asNumber();
        GLdouble top = args[3].asNumber();
        GLdouble zNear = args[4].asNumber();
        GLdouble zFar = args[5].asNumber();
        glOrtho(left, right, bottom, top, zNear, zFar);
        return 0;
    }



    void register_opengl(ModuleBuilder &module)
    {
      

            // =============================================================
            // BASIC OPENGL FUNCTIONS
            // =============================================================
            module.addFunction("glClearColor", native_glClearColor, 4)
            .addFunction("glClear", native_glClear, 1)
            .addFunction("glViewport", native_glViewport, 4)
            .addFunction("glEnable", native_glEnable, 1)
            .addFunction("glDisable", native_glDisable, 1)
            .addFunction("glBlendFunc", native_glBlendFunc, 2)
            .addFunction("glDepthFunc", native_glDepthFunc, 1)
            .addFunction("glDepthMask", native_glDepthMask, 1)
            .addFunction("glCullFace", native_glCullFace, 1)
            .addFunction("glFrontFace", native_glFrontFace, 1)
            .addFunction("glPolygonMode", native_glPolygonMode, 2)
            .addFunction("glLineWidth", native_glLineWidth, 1)
            .addFunction("glPointSize", native_glPointSize, 1)
            .addFunction("glScissor", native_glScissor, 4)
            .addFunction("glGetError", native_glGetError, 0)
            .addFunction("GLDebug", native_GLDebug, 1)
            .addFunction("GLCheck", native_GLCheck, -1)
            .addFunction("glGetString", native_glGetString, 1)
            .addFunction("glGetIntegerv", native_glGetIntegerv, 1)
            .addFunction("glFlush", native_glFlush, 0)
            .addFunction("glFinish", native_glFinish, 0)

            // =============================================================
            // IMMEDIATE MODE (OpenGL 1.x)
            // =============================================================
            .addFunction("glBegin", native_glBegin, 1)
            .addFunction("glEnd", native_glEnd, 0)
            .addFunction("glVertex2f", native_glVertex2f, 2)
            .addFunction("glVertex3f", native_glVertex3f, 3)
            .addFunction("glColor3f", native_glColor3f, 3)
            .addFunction("glColor4f", native_glColor4f, 4)
            .addFunction("glTexCoord2f", native_glTexCoord2f, 2)
            .addFunction("glNormal3f", native_glNormal3f, 3)

            // =============================================================
            // MATRIX OPERATIONS
            // =============================================================
            .addFunction("glMatrixMode", native_glMatrixMode, 1)
            .addFunction("glLoadIdentity", native_glLoadIdentity, 0)
            .addFunction("glPushMatrix", native_glPushMatrix, 0)
            .addFunction("glPopMatrix", native_glPopMatrix, 0)
            .addFunction("glTranslatef", native_glTranslatef, 3)
            .addFunction("glRotatef", native_glRotatef, 4)
            .addFunction("glScalef", native_glScalef, 3)
            .addFunction("glOrtho", native_glOrtho, 6)

            // =============================================================
            // GL CLEAR BUFFER BITS
            // =============================================================
            .addInt("GL_COLOR_BUFFER_BIT", GL_COLOR_BUFFER_BIT)
            .addInt("GL_DEPTH_BUFFER_BIT", GL_DEPTH_BUFFER_BIT)
            .addInt("GL_STENCIL_BUFFER_BIT", GL_STENCIL_BUFFER_BIT)

            // =============================================================
            // GL PRIMITIVE TYPES
            // =============================================================
            .addByte("GL_POINTS", GL_POINTS)
            .addByte("GL_LINES", GL_LINES)
            .addByte("GL_LINE_LOOP", GL_LINE_LOOP)
            .addByte("GL_LINE_STRIP", GL_LINE_STRIP)
            .addByte("GL_TRIANGLES", GL_TRIANGLES)
            .addByte("GL_TRIANGLE_STRIP", GL_TRIANGLE_STRIP)
            .addByte("GL_TRIANGLE_FAN", GL_TRIANGLE_FAN)
            .addByte("GL_QUADS", GL_QUADS)
            .addByte("GL_QUAD_STRIP", GL_QUAD_STRIP)
            .addByte("GL_POLYGON", GL_POLYGON)

            // =============================================================
            // GL ENABLE CAPS
            // =============================================================
            .addInt("GL_DEPTH_TEST", GL_DEPTH_TEST)
            .addInt("GL_BLEND", GL_BLEND)
            .addInt("GL_CULL_FACE", GL_CULL_FACE)
            .addInt("GL_SCISSOR_TEST", GL_SCISSOR_TEST)
            .addInt("GL_TEXTURE_2D", GL_TEXTURE_2D)
            .addInt("GL_LINE_SMOOTH", GL_LINE_SMOOTH)
            .addInt("GL_POINT_SMOOTH", GL_POINT_SMOOTH)

            // =============================================================
            // GL BLEND FUNCTIONS
            // =============================================================
            .addInt("GL_ZERO", GL_ZERO)
            .addInt("GL_ONE", GL_ONE)
            .addInt("GL_SRC_COLOR", GL_SRC_COLOR)
            .addInt("GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR)
            .addInt("GL_SRC_ALPHA", GL_SRC_ALPHA)
            .addInt("GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA)
            .addInt("GL_DST_ALPHA", GL_DST_ALPHA)
            .addInt("GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA)
            .addInt("GL_DST_COLOR", GL_DST_COLOR)
            .addInt("GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR)

            // =============================================================
            // GL DEPTH FUNCTIONS
            // =============================================================
            .addInt("GL_NEVER", GL_NEVER)
            .addInt("GL_LESS", GL_LESS)
            .addInt("GL_EQUAL", GL_EQUAL)
            .addInt("GL_LEQUAL", GL_LEQUAL)
            .addInt("GL_GREATER", GL_GREATER)
            .addInt("GL_NOTEQUAL", GL_NOTEQUAL)
            .addInt("GL_GEQUAL", GL_GEQUAL)
            .addInt("GL_ALWAYS", GL_ALWAYS)

            // =============================================================
            // GL CULL FACE
            // =============================================================
            .addInt("GL_FRONT", GL_FRONT)
            .addInt("GL_BACK", GL_BACK)
            .addInt("GL_FRONT_AND_BACK", GL_FRONT_AND_BACK)

            // =============================================================
            // GL FRONT FACE
            // =============================================================
            .addInt("GL_CW", GL_CW)
            .addInt("GL_CCW", GL_CCW)

            // =============================================================
            // GL POLYGON MODE
            // =============================================================
            .addInt("GL_POINT", GL_POINT)
            .addInt("GL_LINE", GL_LINE)
            .addInt("GL_FILL", GL_FILL)

            // =============================================================
            // GL MATRIX MODE
            // =============================================================
            .addInt("GL_MODELVIEW", GL_MODELVIEW)
            .addInt("GL_PROJECTION", GL_PROJECTION)
            .addInt("GL_TEXTURE", GL_TEXTURE)
            
            // =============================================================
            // GL STRING NAMES
            // =============================================================
            .addInt("GL_VENDOR", GL_VENDOR)
            .addInt("GL_RENDERER", GL_RENDERER)
            .addInt("GL_VERSION", GL_VERSION)
            .addInt("GL_EXTENSIONS", GL_EXTENSIONS)

            // =============================================================
            // GL DATA TYPES
            // =============================================================
            .addInt("GL_BYTE", GL_BYTE)
            .addInt("GL_UNSIGNED_BYTE", GL_UNSIGNED_BYTE)
            .addInt("GL_SHORT", GL_SHORT)
            .addInt("GL_UNSIGNED_SHORT", GL_UNSIGNED_SHORT)
            .addInt("GL_INT", GL_INT)
            .addInt("GL_UNSIGNED_INT", GL_UNSIGNED_INT)
            .addInt("GL_FLOAT", GL_FLOAT)
            .addInt("GL_DOUBLE", GL_DOUBLE)
            
            // =============================================================
            // GL ERROR CODES
            // =============================================================
            .addInt("GL_NO_ERROR", GL_NO_ERROR)
            .addInt("GL_INVALID_ENUM", GL_INVALID_ENUM)
            .addInt("GL_INVALID_VALUE", GL_INVALID_VALUE)
            .addInt("GL_INVALID_OPERATION", GL_INVALID_OPERATION)
            .addInt("GL_OUT_OF_MEMORY", GL_OUT_OF_MEMORY)

            // =============================================================
            // GL BOOLEAN
            // =============================================================
            .addByte("GL_TRUE", GL_TRUE)
            .addByte("GL_FALSE", GL_FALSE);

        // Separate registration blocks (modular organization)
        register_opengl_legacy(module);
        register_opengl_vertexbuffer(module);
        register_opengl_texture(module);
        register_opengl_rendertarget(module);
    }
}
