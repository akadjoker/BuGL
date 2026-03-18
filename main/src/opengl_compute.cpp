#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

namespace SDLBindings
{
    // =====================================================
    // COMPUTE SHADERS
    // =====================================================

    int native_glDispatchCompute(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glDispatchCompute expects 3 arguments (num_groups_x, num_groups_y, num_groups_z)");
            return 0;
        }
        glDispatchCompute((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber(), (GLuint)args[2].asNumber());
        return 0;
    }

    int native_glDispatchComputeIndirect(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDispatchComputeIndirect expects 1 argument (indirect_offset)");
            return 0;
        }
        glDispatchComputeIndirect((GLintptr)args[0].asNumber());
        return 0;
    }

    int native_glMemoryBarrier(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glMemoryBarrier expects 1 argument (barriers)");
            return 0;
        }
        glMemoryBarrier((GLbitfield)args[0].asNumber());
        return 0;
    }

    int native_glMemoryBarrierByRegion(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glMemoryBarrierByRegion expects 1 argument (barriers)");
            return 0;
        }
#ifdef GL_VERSION_4_5
        glMemoryBarrierByRegion((GLbitfield)args[0].asNumber());
#else
        glMemoryBarrier((GLbitfield)args[0].asNumber());
#endif
        return 0;
    }

    // =====================================================
    // SSBO (Shader Storage Buffer Objects)
    // =====================================================

    int native_glShaderStorageBlockBinding(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glShaderStorageBlockBinding expects 3 arguments (program, storageBlockIndex, storageBlockBinding)");
            return 0;
        }
        glShaderStorageBlockBinding((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber(), (GLuint)args[2].asNumber());
        return 0;
    }

    int native_glGetProgramResourceIndex(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[2].isString())
        {
            Error("glGetProgramResourceIndex expects (program, programInterface, name)");
            return 0;
        }
        GLuint idx = glGetProgramResourceIndex((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), args[2].asStringChars());
        vm->pushInt((int)idx);
        return 1;
    }

    // =====================================================
    // IMAGE LOAD/STORE
    // =====================================================

    int native_glBindImageTexture(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 7)
        {
            Error("glBindImageTexture expects 7 arguments (unit, texture, level, layered, layer, access, format)");
            return 0;
        }
        glBindImageTexture((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber(),
                           (GLint)args[2].asNumber(), args[3].asBool() ? GL_TRUE : GL_FALSE,
                           (GLint)args[4].asNumber(), (GLenum)args[5].asNumber(),
                           (GLenum)args[6].asNumber());
        return 0;
    }

    // =====================================================
    // REGISTRATION
    // =====================================================

    void register_opengl_compute(ModuleBuilder &module)
    {
        module
            // Compute shaders
            .addFunction("glDispatchCompute", native_glDispatchCompute, 3)
            .addFunction("glDispatchComputeIndirect", native_glDispatchComputeIndirect, 1)
            .addFunction("glMemoryBarrier", native_glMemoryBarrier, 1)
            .addFunction("glMemoryBarrierByRegion", native_glMemoryBarrierByRegion, 1)

            // SSBO
            .addFunction("glShaderStorageBlockBinding", native_glShaderStorageBlockBinding, 3)
            .addFunction("glGetProgramResourceIndex", native_glGetProgramResourceIndex, 3)

            // Image load/store
            .addFunction("glBindImageTexture", native_glBindImageTexture, 7);

        // Compute shader constants
#ifdef GL_COMPUTE_SHADER
        module.addInt("GL_COMPUTE_SHADER", GL_COMPUTE_SHADER);
#endif
#ifdef GL_MAX_COMPUTE_WORK_GROUP_COUNT
        module.addInt("GL_MAX_COMPUTE_WORK_GROUP_COUNT", GL_MAX_COMPUTE_WORK_GROUP_COUNT);
#endif
#ifdef GL_MAX_COMPUTE_WORK_GROUP_SIZE
        module.addInt("GL_MAX_COMPUTE_WORK_GROUP_SIZE", GL_MAX_COMPUTE_WORK_GROUP_SIZE);
#endif
#ifdef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
        module.addInt("GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS", GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
#endif
#ifdef GL_DISPATCH_INDIRECT_BUFFER
        module.addInt("GL_DISPATCH_INDIRECT_BUFFER", GL_DISPATCH_INDIRECT_BUFFER);
#endif

        // Memory barrier bits
#ifdef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
        module.addInt("GL_SHADER_IMAGE_ACCESS_BARRIER_BIT", GL_SHADER_IMAGE_ACCESS_BARRIER_BIT)
              .addInt("GL_SHADER_STORAGE_BARRIER_BIT", GL_SHADER_STORAGE_BARRIER_BIT)
              .addInt("GL_TEXTURE_FETCH_BARRIER_BIT", GL_TEXTURE_FETCH_BARRIER_BIT)
              .addInt("GL_BUFFER_UPDATE_BARRIER_BIT", GL_BUFFER_UPDATE_BARRIER_BIT)
              .addInt("GL_FRAMEBUFFER_BARRIER_BIT", GL_FRAMEBUFFER_BARRIER_BIT)
              .addInt("GL_ALL_BARRIER_BITS", GL_ALL_BARRIER_BITS)
              .addInt("GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT", GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT)
              .addInt("GL_ELEMENT_ARRAY_BARRIER_BIT", GL_ELEMENT_ARRAY_BARRIER_BIT)
              .addInt("GL_UNIFORM_BARRIER_BIT", GL_UNIFORM_BARRIER_BIT)
              .addInt("GL_TEXTURE_UPDATE_BARRIER_BIT", GL_TEXTURE_UPDATE_BARRIER_BIT)
              .addInt("GL_COMMAND_BARRIER_BIT", GL_COMMAND_BARRIER_BIT)
              .addInt("GL_PIXEL_BUFFER_BARRIER_BIT", GL_PIXEL_BUFFER_BARRIER_BIT);
#endif
#ifdef GL_ATOMIC_COUNTER_BARRIER_BIT
        module.addInt("GL_ATOMIC_COUNTER_BARRIER_BIT", GL_ATOMIC_COUNTER_BARRIER_BIT);
#endif

        // SSBO constants
#ifdef GL_SHADER_STORAGE_BUFFER
        module.addInt("GL_SHADER_STORAGE_BUFFER", GL_SHADER_STORAGE_BUFFER);
#endif
#ifdef GL_SHADER_STORAGE_BUFFER_BINDING
        module.addInt("GL_SHADER_STORAGE_BUFFER_BINDING", GL_SHADER_STORAGE_BUFFER_BINDING);
#endif
#ifdef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
        module.addInt("GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS", GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS);
#endif
#ifdef GL_MAX_SHADER_STORAGE_BLOCK_SIZE
        module.addInt("GL_MAX_SHADER_STORAGE_BLOCK_SIZE", GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
#endif
#ifdef GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT
        module.addInt("GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT", GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
#endif

        // Program resource interface constants
#ifdef GL_SHADER_STORAGE_BLOCK
        module.addInt("GL_SHADER_STORAGE_BLOCK", GL_SHADER_STORAGE_BLOCK);
#endif
#ifdef GL_BUFFER_VARIABLE
        module.addInt("GL_BUFFER_VARIABLE", GL_BUFFER_VARIABLE);
#endif

        // Image load/store constants
#ifdef GL_MAX_IMAGE_UNITS
        module.addInt("GL_MAX_IMAGE_UNITS", GL_MAX_IMAGE_UNITS);
#endif
#ifdef GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS
        module.addInt("GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS", GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS);
#endif

        // Atomic counter buffer
#ifdef GL_ATOMIC_COUNTER_BUFFER
        module.addInt("GL_ATOMIC_COUNTER_BUFFER", GL_ATOMIC_COUNTER_BUFFER);
#endif
#ifdef GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS
        module.addInt("GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS", GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS);
#endif
    }
}
