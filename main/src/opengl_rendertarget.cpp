#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include "stb_image_write.h"
#include <algorithm>
#include <limits>
#include <vector>

namespace SDLBindings
{
    int native_glGenFramebuffers(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenFramebuffers expects 0 arguments");
            return 0;
        }

        GLuint id = 0;
        glGenFramebuffers(1, &id);
        vm->pushInt((int)id);
        return 1;
    }

    int native_glBindFramebuffer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glBindFramebuffer expects 2 arguments: target, framebuffer");
            return 0;
        }

        glBindFramebuffer((GLenum)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glDeleteFramebuffers(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteFramebuffers expects 1 argument: framebuffer");
            return 0;
        }

        const GLuint id = (GLuint)args[0].asNumber();
        glDeleteFramebuffers(1, &id);
        return 0;
    }

    int native_glCheckFramebufferStatus(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glCheckFramebufferStatus expects 1 argument: target");
            return 0;
        }

        const GLenum status = glCheckFramebufferStatus((GLenum)args[0].asNumber());
        vm->pushInt((int)status);
        return 1;
    }

    int native_glFramebufferTexture2D(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glFramebufferTexture2D expects 5 arguments: target, attachment, textarget, texture, level");
            return 0;
        }

        glFramebufferTexture2D((GLenum)args[0].asNumber(),
                               (GLenum)args[1].asNumber(),
                               (GLenum)args[2].asNumber(),
                               (GLuint)args[3].asNumber(),
                               (GLint)args[4].asNumber());
        return 0;
    }

    int native_glFramebufferRenderbuffer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glFramebufferRenderbuffer expects 4 arguments: target, attachment, renderbuffertarget, renderbuffer");
            return 0;
        }

        glFramebufferRenderbuffer((GLenum)args[0].asNumber(),
                                  (GLenum)args[1].asNumber(),
                                  (GLenum)args[2].asNumber(),
                                  (GLuint)args[3].asNumber());
        return 0;
    }

    int native_glDrawBuffers(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2 || !args[0].isNumber() || !args[1].isArray())
        {
            Error("glDrawBuffers expects (count, [attachments...])");
            return 0;
        }

        const int n = args[0].asInt();
        if (n <= 0)
        {
            Error("glDrawBuffers count must be > 0");
            return 0;
        }

        ArrayInstance *arr = args[1].asArray();
        if (!arr)
        {
            Error("glDrawBuffers invalid attachments array");
            return 0;
        }

        if ((int)arr->values.size() < n)
        {
            Error("glDrawBuffers array must contain at least count attachments");
            return 0;
        }

        std::vector<GLenum> bufs((size_t)n);
        for (int i = 0; i < n; i++)
        {
            if (!arr->values[i].isNumber())
            {
                Error("glDrawBuffers attachments must be numeric enums");
                return 0;
            }
            bufs[(size_t)i] = (GLenum)arr->values[i].asInt();
        }

        glDrawBuffers((GLsizei)n, bufs.data());
        return 0;
    }

    int native_glGenRenderbuffers(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenRenderbuffers expects 0 arguments");
            return 0;
        }

        GLuint id = 0;
        glGenRenderbuffers(1, &id);
        vm->pushInt((int)id);
        return 1;
    }

    int native_glBindRenderbuffer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glBindRenderbuffer expects 2 arguments: target, renderbuffer");
            return 0;
        }

        glBindRenderbuffer((GLenum)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glDeleteRenderbuffers(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteRenderbuffers expects 1 argument: renderbuffer");
            return 0;
        }

        const GLuint id = (GLuint)args[0].asNumber();
        glDeleteRenderbuffers(1, &id);
        return 0;
    }

    int native_glRenderbufferStorage(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("glRenderbufferStorage expects 4 arguments: target, internalFormat, width, height");
            return 0;
        }

        glRenderbufferStorage((GLenum)args[0].asNumber(),
                              (GLenum)args[1].asNumber(),
                              (GLsizei)args[2].asNumber(),
                              (GLsizei)args[3].asNumber());
        return 0;
    }

    int native_glGenerateMipmap(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glGenerateMipmap expects 1 argument: target");
            return 0;
        }

        glGenerateMipmap((GLenum)args[0].asNumber());
        return 0;
    }

    int native_glRenderbufferStorageMultisample(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5)
        {
            Error("glRenderbufferStorageMultisample expects 5 arguments: target, samples, internalFormat, width, height");
            return 0;
        }

        glRenderbufferStorageMultisample((GLenum)args[0].asNumber(),
                                         (GLsizei)args[1].asNumber(),
                                         (GLenum)args[2].asNumber(),
                                         (GLsizei)args[3].asNumber(),
                                         (GLsizei)args[4].asNumber());
        return 0;
    }

    int native_glBlitFramebuffer(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 10)
        {
            Error("glBlitFramebuffer expects 10 arguments");
            return 0;
        }

        glBlitFramebuffer((GLint)args[0].asNumber(), (GLint)args[1].asNumber(),
                          (GLint)args[2].asNumber(), (GLint)args[3].asNumber(),
                          (GLint)args[4].asNumber(), (GLint)args[5].asNumber(),
                          (GLint)args[6].asNumber(), (GLint)args[7].asNumber(),
                          (GLbitfield)args[8].asNumber(), (GLenum)args[9].asNumber());
        return 0;
    }

    static bool capture_screenshot_png(const char *path, GLint x, GLint y, GLsizei width, GLsizei height, bool flipY)
    {
        if (width <= 0 || height <= 0)
        {
            return false;
        }

        const size_t rowBytes = (size_t)width * 4;
        if ((size_t)width > (std::numeric_limits<size_t>::max() / 4) ||
            (size_t)height > (std::numeric_limits<size_t>::max() / rowBytes))
        {
            return false;
        }
        const size_t totalBytes = rowBytes * (size_t)height;

        std::vector<unsigned char> pixels(totalBytes);

        GLint oldPack = 4;
        glGetIntegerv(GL_PACK_ALIGNMENT, &oldPack);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glPixelStorei(GL_PACK_ALIGNMENT, oldPack);

        if (flipY && height > 1)
        {
            std::vector<unsigned char> row(rowBytes);
            const int half = height / 2;
            for (int j = 0; j < half; j++)
            {
                unsigned char *top = pixels.data() + ((size_t)j * rowBytes);
                unsigned char *bottom = pixels.data() + ((size_t)(height - 1 - j) * rowBytes);
                std::copy(top, top + rowBytes, row.data());
                std::copy(bottom, bottom + rowBytes, top);
                std::copy(row.data(), row.data() + rowBytes, bottom);
            }
        }

        const int ok = stbi_write_png(path, width, height, 4, pixels.data(), (int)rowBytes);
        return ok != 0;
    }

    int native_glCaptureScreenshotPNG(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5 && argc != 6)
        {
            Error("glCaptureScreenshotPNG expects (path, x, y, width, height[, flipY])");
            return 0;
        }
        if (!args[0].isString())
        {
            Error("glCaptureScreenshotPNG argument 1 must be path string");
            return 0;
        }

        const char *path = args[0].asStringChars();
        const GLint x = (GLint)args[1].asNumber();
        const GLint y = (GLint)args[2].asNumber();
        const GLsizei width = (GLsizei)args[3].asNumber();
        const GLsizei height = (GLsizei)args[4].asNumber();
        const bool flipY = (argc == 6) ? args[5].asBool() : true;

        if (width <= 0 || height <= 0)
        {
            Error("glCaptureScreenshotPNG width/height must be > 0");
            return 0;
        }

        if (!capture_screenshot_png(path, x, y, width, height, flipY))
        {
            Error("glCaptureScreenshotPNG failed");
            return 0;
        }

        vm->pushInt(1);
        return 1;
    }

    int native_SaveScreenshot(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 && argc != 2)
        {
            Error("SaveScreenshot expects (path[, flipY])");
            return 0;
        }
        if (!args[0].isString())
        {
            Error("SaveScreenshot argument 1 must be path string");
            return 0;
        }

        const char *path = args[0].asStringChars();
        const bool flipY = (argc == 2) ? args[1].asBool() : true;

        GLint vp[4] = {0, 0, 0, 0};
        glGetIntegerv(GL_VIEWPORT, vp);
        const GLint x = vp[0];
        const GLint y = vp[1];
        const GLsizei width = (GLsizei)vp[2];
        const GLsizei height = (GLsizei)vp[3];

        if (!capture_screenshot_png(path, x, y, width, height, flipY))
        {
            Error("SaveScreenshot failed");
            return 0;
        }

        vm->pushInt(1);
        return 1;
    }

    void register_opengl_rendertarget(ModuleBuilder &module)
    {
        module.addFunction("glGenFramebuffers", native_glGenFramebuffers, 0)
            .addFunction("glBindFramebuffer", native_glBindFramebuffer, 2)
            .addFunction("glDeleteFramebuffers", native_glDeleteFramebuffers, 1)
            .addFunction("glCheckFramebufferStatus", native_glCheckFramebufferStatus, 1)
            .addFunction("glFramebufferTexture2D", native_glFramebufferTexture2D, 5)
            .addFunction("glFramebufferRenderbuffer", native_glFramebufferRenderbuffer, 4)
            .addFunction("glDrawBuffers", native_glDrawBuffers, 2)
            .addFunction("glGenRenderbuffers", native_glGenRenderbuffers, 0)
            .addFunction("glBindRenderbuffer", native_glBindRenderbuffer, 2)
            .addFunction("glDeleteRenderbuffers", native_glDeleteRenderbuffers, 1)
            .addFunction("glRenderbufferStorage", native_glRenderbufferStorage, 4)
            .addFunction("glGenerateMipmap", native_glGenerateMipmap, 1)
            .addFunction("glRenderbufferStorageMultisample", native_glRenderbufferStorageMultisample, 5)
            .addFunction("glBlitFramebuffer", native_glBlitFramebuffer, 10)
            .addFunction("glCaptureScreenshotPNG", native_glCaptureScreenshotPNG, -1)
            .addFunction("SaveScreenshot", native_SaveScreenshot, -1)

            .addInt("GL_FRAMEBUFFER", GL_FRAMEBUFFER)
            .addInt("GL_RENDERBUFFER", GL_RENDERBUFFER)
            .addInt("GL_FRAMEBUFFER_BINDING", GL_FRAMEBUFFER_BINDING)
            .addInt("GL_RENDERBUFFER_BINDING", GL_RENDERBUFFER_BINDING)
            .addInt("GL_COLOR_ATTACHMENT0", GL_COLOR_ATTACHMENT0)
            .addInt("GL_DEPTH_ATTACHMENT", GL_DEPTH_ATTACHMENT)
            .addInt("GL_STENCIL_ATTACHMENT", GL_STENCIL_ATTACHMENT)
            .addInt("GL_FRAMEBUFFER_COMPLETE", GL_FRAMEBUFFER_COMPLETE);

#ifdef GL_DEPTH_STENCIL_ATTACHMENT
        module.addInt("GL_DEPTH_STENCIL_ATTACHMENT", GL_DEPTH_STENCIL_ATTACHMENT);
#endif
#ifdef GL_FRAMEBUFFER_UNDEFINED
        module.addInt("GL_FRAMEBUFFER_UNDEFINED", GL_FRAMEBUFFER_UNDEFINED);
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
        module.addInt("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT", GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
        module.addInt("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT", GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
        module.addInt("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER", GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
        module.addInt("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER", GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
#endif
#ifdef GL_FRAMEBUFFER_UNSUPPORTED
        module.addInt("GL_FRAMEBUFFER_UNSUPPORTED", GL_FRAMEBUFFER_UNSUPPORTED);
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
        module.addInt("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE", GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
        module.addInt("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS", GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
#endif
#ifdef GL_DRAW_FRAMEBUFFER
        module.addInt("GL_DRAW_FRAMEBUFFER", GL_DRAW_FRAMEBUFFER);
#endif
#ifdef GL_READ_FRAMEBUFFER
        module.addInt("GL_READ_FRAMEBUFFER", GL_READ_FRAMEBUFFER);
#endif
#ifdef GL_COLOR_ATTACHMENT1
        module.addInt("GL_COLOR_ATTACHMENT1", GL_COLOR_ATTACHMENT1);
#endif
#ifdef GL_COLOR_ATTACHMENT2
        module.addInt("GL_COLOR_ATTACHMENT2", GL_COLOR_ATTACHMENT2);
#endif
#ifdef GL_COLOR_ATTACHMENT3
        module.addInt("GL_COLOR_ATTACHMENT3", GL_COLOR_ATTACHMENT3);
#endif
#ifdef GL_COLOR_ATTACHMENT4
        module.addInt("GL_COLOR_ATTACHMENT4", GL_COLOR_ATTACHMENT4);
#endif
#ifdef GL_COLOR_ATTACHMENT5
        module.addInt("GL_COLOR_ATTACHMENT5", GL_COLOR_ATTACHMENT5);
#endif
#ifdef GL_COLOR_ATTACHMENT6
        module.addInt("GL_COLOR_ATTACHMENT6", GL_COLOR_ATTACHMENT6);
#endif
#ifdef GL_COLOR_ATTACHMENT7
        module.addInt("GL_COLOR_ATTACHMENT7", GL_COLOR_ATTACHMENT7);
#endif
#ifdef GL_DEPTH_COMPONENT16
        module.addInt("GL_DEPTH_COMPONENT16", GL_DEPTH_COMPONENT16);
#endif
#ifdef GL_DEPTH_COMPONENT24
        module.addInt("GL_DEPTH_COMPONENT24", GL_DEPTH_COMPONENT24);
#endif
#ifdef GL_DEPTH_COMPONENT32
        module.addInt("GL_DEPTH_COMPONENT32", GL_DEPTH_COMPONENT32);
#endif
#ifdef GL_DEPTH24_STENCIL8
        module.addInt("GL_DEPTH24_STENCIL8", GL_DEPTH24_STENCIL8);
#endif
#ifdef GL_DEPTH_STENCIL
        module.addInt("GL_DEPTH_STENCIL", GL_DEPTH_STENCIL);
#endif
#ifdef GL_RGBA8
        module.addInt("GL_RGBA8", GL_RGBA8);
#endif
#ifdef GL_RGB8
        module.addInt("GL_RGB8", GL_RGB8);
#endif
#ifdef GL_MAX_COLOR_ATTACHMENTS
        module.addInt("GL_MAX_COLOR_ATTACHMENTS", GL_MAX_COLOR_ATTACHMENTS);
#endif
#ifdef GL_RENDERBUFFER_SAMPLES
        module.addInt("GL_RENDERBUFFER_SAMPLES", GL_RENDERBUFFER_SAMPLES);
#endif
#ifdef GL_MAX_SAMPLES
        module.addInt("GL_MAX_SAMPLES", GL_MAX_SAMPLES);
#endif
#ifdef GL_READ_FRAMEBUFFER_BINDING
        module.addInt("GL_READ_FRAMEBUFFER_BINDING", GL_READ_FRAMEBUFFER_BINDING);
#endif
#ifdef GL_DRAW_FRAMEBUFFER_BINDING
        module.addInt("GL_DRAW_FRAMEBUFFER_BINDING", GL_DRAW_FRAMEBUFFER_BINDING);
#endif
    }
}
