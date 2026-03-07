#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <limits>
#include <vector>

namespace SDLBindings
{
    static bool checked_mul_size(size_t a, size_t b, size_t *out)
    {
        if (!out)
            return false;
        if (a != 0 && b > (std::numeric_limits<size_t>::max() / a))
            return false;
        *out = a * b;
        return true;
    }

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

    static int glFormatComponentCount(GLenum format)
    {
        switch (format)
        {
        case GL_ALPHA:
        case GL_LUMINANCE:
            return 1;
        case GL_LUMINANCE_ALPHA:
            return 2;
        case GL_RGB:
#ifdef GL_BGR
        case GL_BGR:
#endif
            return 3;
        case GL_RGBA:
#ifdef GL_BGRA
        case GL_BGRA:
#endif
            return 4;
        default:
            return 0;
        }
    }

    static int glPackedPixelSize(GLenum type)
    {
        switch (type)
        {
#ifdef GL_UNSIGNED_SHORT_4_4_4_4
        case GL_UNSIGNED_SHORT_4_4_4_4:
#endif
#ifdef GL_UNSIGNED_SHORT_5_5_5_1
        case GL_UNSIGNED_SHORT_5_5_5_1:
#endif
#ifdef GL_UNSIGNED_SHORT_5_6_5
        case GL_UNSIGNED_SHORT_5_6_5:
#endif
            return 2;

#ifdef GL_UNSIGNED_INT_8_8_8_8
        case GL_UNSIGNED_INT_8_8_8_8:
#endif
#ifdef GL_UNSIGNED_INT_10_10_10_2
        case GL_UNSIGNED_INT_10_10_10_2:
#endif
            return 4;
        default:
            return 0;
        }
    }

    static int glTypeByteSize(GLenum type)
    {
        switch (type)
        {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
            return 1;
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
            return 2;
        case GL_UNSIGNED_INT:
        case GL_INT:
        case GL_FLOAT:
            return 4;
        case GL_DOUBLE:
            return 8;
        default:
            return 0;
        }
    }

    static bool computeTextureUploadSize(GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLenum type,
                                         GLsizeiptr *outSize)
    {
        if (!outSize || width < 0 || height < 0)
            return false;

        size_t pixels = 0;
        if (!checked_mul_size((size_t)width, (size_t)height, &pixels))
            return false;

        int packedSize = glPackedPixelSize(type);
        size_t bytes = 0;
        if (packedSize > 0)
        {
            if (!checked_mul_size(pixels, (size_t)packedSize, &bytes))
                return false;
        }
        else
        {
            const int comps = glFormatComponentCount(format);
            const int typeSize = glTypeByteSize(type);
            if (comps <= 0 || typeSize <= 0)
                return false;

            size_t bpp = 0;
            if (!checked_mul_size((size_t)comps, (size_t)typeSize, &bpp))
                return false;
            if (!checked_mul_size(pixels, bpp, &bytes))
                return false;
        }

        if (bytes > (size_t)std::numeric_limits<GLsizeiptr>::max())
            return false;

        *outSize = (GLsizeiptr)bytes;
        return true;
    }

    static bool resolveTextureDataArg(const Value &value,
                                      GLsizei width,
                                      GLsizei height,
                                      GLenum format,
                                      GLenum type,
                                      const char *fnName,
                                      const GLvoid **outPtr,
                                      std::vector<unsigned char> &scratch)
    {
        GLsizeiptr expectedSize = 0;
        const bool hasExpectedSize = computeTextureUploadSize(width, height, format, type, &expectedSize);

        if (value.isBuffer() && hasExpectedSize)
        {
            BufferInstance *buf = value.asBuffer();
            if (buf)
            {
                size_t haveBytes = 0;
                if (!checked_mul_size((size_t)buf->count, (size_t)buf->elementSize, &haveBytes))
                {
                    Error("%s buffer size overflow", fnName);
                    return false;
                }
                if (haveBytes < (size_t)expectedSize)
                {
                    Error("%s buffer too small: need %lld bytes", fnName, (long long)expectedSize);
                    return false;
                }
            }
        }

        if (value.isArray() && !hasExpectedSize)
        {
            Error("%s array upload needs known format/type pair", fnName);
            return false;
        }

        if (hasExpectedSize)
            return resolveUploadDataArg(value, expectedSize, fnName, outPtr, scratch);

        return resolveUploadDataArg(value, 0, fnName, outPtr, scratch);
    }

    int native_glGenTextures(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenTextures expects 0 arguments");
            return 0;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        vm->pushInt((int)tex);
        return 1;
    }

    int native_glDeleteTextures(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteTextures expects 1 argument");
            return 0;
        }

        const GLuint tex = (GLuint)args[0].asNumber();
        glDeleteTextures(1, &tex);
        return 0;
    }

    int native_glBindTexture(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glBindTexture expects 2 arguments: target, texture");
            return 0;
        }

        glBindTexture((GLenum)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glPixelStorei(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glPixelStorei expects 2 arguments: pname, param");
            return 0;
        }

        glPixelStorei((GLenum)args[0].asNumber(), (GLint)args[1].asNumber());
        return 0;
    }

    int native_glTexParameteri(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glTexParameteri expects 3 arguments: target, pname, param");
            return 0;
        }

        glTexParameteri((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLint)args[2].asNumber());
        return 0;
    }

    int native_glTexParameterf(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glTexParameterf expects 3 arguments: target, pname, param");
            return 0;
        }

        glTexParameterf((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLfloat)args[2].asNumber());
        return 0;
    }

    int native_glTexEnvi(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("glTexEnvi expects 3 arguments: target, pname, param");
            return 0;
        }

        glTexEnvi((GLenum)args[0].asNumber(), (GLenum)args[1].asNumber(), (GLint)args[2].asNumber());
        return 0;
    }

    int native_glTexImage2D(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 9)
        {
            Error("glTexImage2D expects 9 arguments: target, level, internalFormat, width, height, border, format, type, data");
            return 0;
        }

        const GLenum target = (GLenum)args[0].asNumber();
        const GLint level = (GLint)args[1].asNumber();
        const GLint internalFormat = (GLint)args[2].asNumber();
        const GLsizei width = (GLsizei)args[3].asNumber();
        const GLsizei height = (GLsizei)args[4].asNumber();
        const GLint border = (GLint)args[5].asNumber();
        const GLenum format = (GLenum)args[6].asNumber();
        const GLenum type = (GLenum)args[7].asNumber();

        const GLvoid *pixels = nullptr;
        std::vector<unsigned char> scratch;
        if (!resolveTextureDataArg(args[8], width, height, format, type, "glTexImage2D", &pixels, scratch))
            return 0;

        glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
        return 0;
    }

    int native_glTexSubImage2D(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 9)
        {
            Error("glTexSubImage2D expects 9 arguments: target, level, xoffset, yoffset, width, height, format, type, data");
            return 0;
        }

        const GLenum target = (GLenum)args[0].asNumber();
        const GLint level = (GLint)args[1].asNumber();
        const GLint xoffset = (GLint)args[2].asNumber();
        const GLint yoffset = (GLint)args[3].asNumber();
        const GLsizei width = (GLsizei)args[4].asNumber();
        const GLsizei height = (GLsizei)args[5].asNumber();
        const GLenum format = (GLenum)args[6].asNumber();
        const GLenum type = (GLenum)args[7].asNumber();

        const GLvoid *pixels = nullptr;
        std::vector<unsigned char> scratch;
        if (!resolveTextureDataArg(args[8], width, height, format, type, "glTexSubImage2D", &pixels, scratch))
            return 0;

        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
        return 0;
    }

    void register_opengl_texture(ModuleBuilder &module)
    {
        module.addFunction("glGenTextures", native_glGenTextures, 0)
            .addFunction("glDeleteTextures", native_glDeleteTextures, 1)
            .addFunction("glBindTexture", native_glBindTexture, 2)
            .addFunction("glPixelStorei", native_glPixelStorei, 2)
            .addFunction("glTexParameteri", native_glTexParameteri, 3)
            .addFunction("glTexParameterf", native_glTexParameterf, 3)
            .addFunction("glTexEnvi", native_glTexEnvi, 3)
            .addFunction("glTexImage2D", native_glTexImage2D, 9)
            .addFunction("glTexSubImage2D", native_glTexSubImage2D, 9)

            .addInt("GL_TEXTURE_MIN_FILTER", GL_TEXTURE_MIN_FILTER)
            .addInt("GL_TEXTURE_MAG_FILTER", GL_TEXTURE_MAG_FILTER)
            .addInt("GL_TEXTURE_WRAP_S", GL_TEXTURE_WRAP_S)
            .addInt("GL_TEXTURE_WRAP_T", GL_TEXTURE_WRAP_T)
            .addInt("GL_NEAREST", GL_NEAREST)
            .addInt("GL_LINEAR", GL_LINEAR)
            .addInt("GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST)
            .addInt("GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR)
            .addInt("GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST)
            .addInt("GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR)
            .addInt("GL_REPEAT", GL_REPEAT)
            .addInt("GL_RGB", GL_RGB)
            .addInt("GL_RGBA", GL_RGBA)
            .addInt("GL_LUMINANCE", GL_LUMINANCE)
            .addInt("GL_LUMINANCE_ALPHA", GL_LUMINANCE_ALPHA)
            .addInt("GL_ALPHA", GL_ALPHA)
            .addInt("GL_UNPACK_ALIGNMENT", GL_UNPACK_ALIGNMENT)
            .addInt("GL_PACK_ALIGNMENT", GL_PACK_ALIGNMENT)
            .addInt("GL_TEXTURE_ENV", GL_TEXTURE_ENV)
            .addInt("GL_TEXTURE_ENV_MODE", GL_TEXTURE_ENV_MODE)
            .addInt("GL_MODULATE", GL_MODULATE)
            .addInt("GL_REPLACE", GL_REPLACE)
            .addInt("GL_RGBA16F", GL_RGBA16F)
            .addInt("GL_RGBA32F", GL_RGBA32F)
            .addInt("GL_RGB16F", GL_RGB16F)
            .addInt("GL_RGB32F", GL_RGB32F);
            

#ifdef GL_CLAMP
        module.addInt("GL_CLAMP", GL_CLAMP);
#endif
#ifdef GL_CLAMP_TO_EDGE
        module.addInt("GL_CLAMP_TO_EDGE", GL_CLAMP_TO_EDGE);
#endif
#ifdef GL_MIRRORED_REPEAT
        module.addInt("GL_MIRRORED_REPEAT", GL_MIRRORED_REPEAT);
#endif
#ifdef GL_BGR
        module.addInt("GL_BGR", GL_BGR);
#endif
#ifdef GL_BGRA
        module.addInt("GL_BGRA", GL_BGRA);
#endif
#ifdef GL_DECAL
        module.addInt("GL_DECAL", GL_DECAL);
#endif
#ifdef GL_ADD
        module.addInt("GL_ADD", GL_ADD);
#endif
#ifdef GL_UNSIGNED_SHORT_4_4_4_4
        module.addInt("GL_UNSIGNED_SHORT_4_4_4_4", GL_UNSIGNED_SHORT_4_4_4_4);
#endif
#ifdef GL_UNSIGNED_SHORT_5_5_5_1
        module.addInt("GL_UNSIGNED_SHORT_5_5_5_1", GL_UNSIGNED_SHORT_5_5_5_1);
#endif
#ifdef GL_UNSIGNED_SHORT_5_6_5
        module.addInt("GL_UNSIGNED_SHORT_5_6_5", GL_UNSIGNED_SHORT_5_6_5);
#endif
#ifdef GL_UNSIGNED_INT_8_8_8_8
        module.addInt("GL_UNSIGNED_INT_8_8_8_8", GL_UNSIGNED_INT_8_8_8_8);
#endif
#ifdef GL_UNSIGNED_INT_10_10_10_2
        module.addInt("GL_UNSIGNED_INT_10_10_10_2", GL_UNSIGNED_INT_10_10_10_2);
#endif
    }
}
