#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <vector>
#include <string>

namespace SDLBindings
{
    // =====================================================
    // TRANSFORM FEEDBACK
    // =====================================================

    int native_glBeginTransformFeedback(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glBeginTransformFeedback expects 1 argument (primitiveMode)");
            return 0;
        }
        glBeginTransformFeedback((GLenum)args[0].asNumber());
        return 0;
    }

    int native_glEndTransformFeedback(Interpreter *vm, int argc, Value *args)
    {
        (void)vm; (void)args;
        glEndTransformFeedback();
        return 0;
    }

    int native_glTransformFeedbackVaryings(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glTransformFeedbackVaryings expects (program, varyingsArray, bufferMode)");
            return 0;
        }

        GLuint program = (GLuint)args[0].asNumber();
        GLenum bufferMode = (GLenum)args[2].asNumber();

        if (!args[1].isArray())
        {
            Error("glTransformFeedbackVaryings: varyings must be an array of strings");
            return 0;
        }
        ArrayInstance *arr = args[1].asArray();
        if (!arr)
        {
            Error("glTransformFeedbackVaryings: invalid array");
            return 0;
        }

        std::vector<std::string> names;
        std::vector<const char *> ptrs;
        names.reserve(arr->values.size());
        ptrs.reserve(arr->values.size());

        for (size_t i = 0; i < arr->values.size(); i++)
        {
            if (!arr->values[i].isString())
            {
                Error("glTransformFeedbackVaryings: all varyings must be strings");
                return 0;
            }
            names.push_back(arr->values[i].asStringChars());
            ptrs.push_back(names.back().c_str());
        }

        glTransformFeedbackVaryings(program, (GLsizei)ptrs.size(), ptrs.data(), bufferMode);
        return 0;
    }

    int native_glGenTransformFeedbacks(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenTransformFeedbacks expects 0 arguments");
            return 0;
        }
        GLuint id = 0;
        glGenTransformFeedbacks(1, &id);
        vm->pushInt((int)id);
        return 1;
    }

    int native_glBindTransformFeedback(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glBindTransformFeedback expects 2 arguments (target, id)");
            return 0;
        }
        glBindTransformFeedback((GLenum)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glDeleteTransformFeedbacks(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteTransformFeedbacks expects 1 argument");
            return 0;
        }
        GLuint id = (GLuint)args[0].asNumber();
        glDeleteTransformFeedbacks(1, &id);
        return 0;
    }

    int native_glPauseTransformFeedback(Interpreter *vm, int argc, Value *args)
    {
        (void)vm; (void)args;
        glPauseTransformFeedback();
        return 0;
    }

    int native_glResumeTransformFeedback(Interpreter *vm, int argc, Value *args)
    {
        (void)vm; (void)args;
        glResumeTransformFeedback();
        return 0;
    }

    // =====================================================
    // TESSELLATION
    // =====================================================

    int native_glPatchParameteri(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glPatchParameteri expects 2 arguments (pname, value)");
            return 0;
        }
        glPatchParameteri((GLenum)args[0].asNumber(), (GLint)args[1].asNumber());
        return 0;
    }

    int native_glPatchParameterfv(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2 || !args[1].isArray())
        {
            Error("glPatchParameterfv expects (pname, valuesArray)");
            return 0;
        }
        GLenum pname = (GLenum)args[0].asNumber();
        ArrayInstance *arr = args[1].asArray();
        if (!arr)
        {
            Error("glPatchParameterfv: invalid array");
            return 0;
        }
        std::vector<GLfloat> values(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); i++)
            values[i] = (GLfloat)arr->values[i].asNumber();

        glPatchParameterfv(pname, values.data());
        return 0;
    }

    // =====================================================
    // REGISTRATION
    // =====================================================

    void register_opengl_transform_feedback(ModuleBuilder &module)
    {
        module
            // Transform feedback
            .addFunction("glBeginTransformFeedback", native_glBeginTransformFeedback, 1)
            .addFunction("glEndTransformFeedback", native_glEndTransformFeedback, 0)
            .addFunction("glTransformFeedbackVaryings", native_glTransformFeedbackVaryings, 3)
            .addFunction("glGenTransformFeedbacks", native_glGenTransformFeedbacks, 0)
            .addFunction("glBindTransformFeedback", native_glBindTransformFeedback, 2)
            .addFunction("glDeleteTransformFeedbacks", native_glDeleteTransformFeedbacks, 1)
            .addFunction("glPauseTransformFeedback", native_glPauseTransformFeedback, 0)
            .addFunction("glResumeTransformFeedback", native_glResumeTransformFeedback, 0)

            // Tessellation
            .addFunction("glPatchParameteri", native_glPatchParameteri, 2)
            .addFunction("glPatchParameterfv", native_glPatchParameterfv, 2);

        // Transform feedback constants
#ifdef GL_TRANSFORM_FEEDBACK_BUFFER
        module.addInt("GL_TRANSFORM_FEEDBACK_BUFFER", GL_TRANSFORM_FEEDBACK_BUFFER);
#endif
#ifdef GL_INTERLEAVED_ATTRIBS
        module.addInt("GL_INTERLEAVED_ATTRIBS", GL_INTERLEAVED_ATTRIBS);
#endif
#ifdef GL_SEPARATE_ATTRIBS
        module.addInt("GL_SEPARATE_ATTRIBS", GL_SEPARATE_ATTRIBS);
#endif
#ifdef GL_TRANSFORM_FEEDBACK
        module.addInt("GL_TRANSFORM_FEEDBACK", GL_TRANSFORM_FEEDBACK);
#endif
#ifdef GL_TRANSFORM_FEEDBACK_BINDING
        module.addInt("GL_TRANSFORM_FEEDBACK_BINDING", GL_TRANSFORM_FEEDBACK_BINDING);
#endif
#ifdef GL_RASTERIZER_DISCARD
        module.addInt("GL_RASTERIZER_DISCARD", GL_RASTERIZER_DISCARD);
#endif
#ifdef GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
        module.addInt("GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN", GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
#endif
#ifdef GL_TRANSFORM_FEEDBACK_BUFFER_BINDING
        module.addInt("GL_TRANSFORM_FEEDBACK_BUFFER_BINDING", GL_TRANSFORM_FEEDBACK_BUFFER_BINDING);
#endif

        // Tessellation constants
#ifdef GL_TESS_CONTROL_SHADER
        module.addInt("GL_TESS_CONTROL_SHADER", GL_TESS_CONTROL_SHADER);
#endif
#ifdef GL_TESS_EVALUATION_SHADER
        module.addInt("GL_TESS_EVALUATION_SHADER", GL_TESS_EVALUATION_SHADER);
#endif
#ifdef GL_PATCHES
        module.addInt("GL_PATCHES", GL_PATCHES);
#endif
#ifdef GL_PATCH_VERTICES
        module.addInt("GL_PATCH_VERTICES", GL_PATCH_VERTICES);
#endif
#ifdef GL_PATCH_DEFAULT_INNER_LEVEL
        module.addInt("GL_PATCH_DEFAULT_INNER_LEVEL", GL_PATCH_DEFAULT_INNER_LEVEL);
#endif
#ifdef GL_PATCH_DEFAULT_OUTER_LEVEL
        module.addInt("GL_PATCH_DEFAULT_OUTER_LEVEL", GL_PATCH_DEFAULT_OUTER_LEVEL);
#endif
#ifdef GL_MAX_PATCH_VERTICES
        module.addInt("GL_MAX_PATCH_VERTICES", GL_MAX_PATCH_VERTICES);
#endif
#ifdef GL_MAX_TESS_GEN_LEVEL
        module.addInt("GL_MAX_TESS_GEN_LEVEL", GL_MAX_TESS_GEN_LEVEL);
#endif
    }
}
