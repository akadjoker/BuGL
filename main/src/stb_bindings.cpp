#include "bindings.hpp"
// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_RESIZE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define STB_PERLIN_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"
#include "stb_perlin.h"
#include <cstring>
#include <limits>

namespace SDLBindings
{
    static int stbi_sdl_read(void *user, char *data, int size)
    {
        SDL_RWops *rw = (SDL_RWops *)user;
        if (!rw || !data || size <= 0)
            return 0;
        return (int)SDL_RWread(rw, data, 1, (size_t)size);
    }

    static void stbi_sdl_skip(void *user, int n)
    {
        SDL_RWops *rw = (SDL_RWops *)user;
        if (!rw || n == 0)
            return;
        SDL_RWseek(rw, n, RW_SEEK_CUR);
    }

    static int stbi_sdl_eof(void *user)
    {
        SDL_RWops *rw = (SDL_RWops *)user;
        if (!rw)
            return 1;

        const Sint64 size = SDL_RWsize(rw);
        if (size < 0)
            return 0;

        const Sint64 pos = SDL_RWtell(rw);
        if (pos < 0)
            return 1;

        return pos >= size ? 1 : 0;
    }

    static stbi_io_callbacks s_stbi_sdl_callbacks = {
        stbi_sdl_read,
        stbi_sdl_skip,
        stbi_sdl_eof};

    struct StbiSdlWriteContext
    {
        SDL_RWops *rw;
        int failed;
    };

    static void stbi_sdl_write(void *context, void *data, int size)
    {
        StbiSdlWriteContext *ctx = (StbiSdlWriteContext *)context;
        if (!ctx || !ctx->rw || size < 0)
        {
            if (ctx)
                ctx->failed = 1;
            return;
        }
        if (size == 0)
            return;
        if (!data)
        {
            ctx->failed = 1;
            return;
        }

        const size_t wrote = SDL_RWwrite(ctx->rw, data, 1, (size_t)size);
        if (wrote != (size_t)size)
            ctx->failed = 1;
    }

    static bool checked_mul_size(size_t a, size_t b, size_t *out)
    {
        if (!out)
            return false;
        if (a != 0 && b > (std::numeric_limits<size_t>::max() / a))
            return false;
        *out = a * b;
        return true;
    }

    static bool resolve_image_data_arg(const Value &value, const void **outData, size_t *outBytes, bool *outKnownBytes)
    {
        if (!outData || !outBytes || !outKnownBytes)
            return false;

        *outData = nullptr;
        *outBytes = 0;
        *outKnownBytes = false;

        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            if (!buf || !buf->data)
            {
                *outData = nullptr;
                *outBytes = 0;
                *outKnownBytes = true;
                return true;
            }
            size_t bytes = 0;
            if (!checked_mul_size((size_t)buf->count, (size_t)buf->elementSize, &bytes))
                return false;
            *outData = buf->data;
            *outBytes = bytes;
            *outKnownBytes = true;
            return true;
        }

        if (getBuiltinTypedArrayData(value, outData))
        {
            *outBytes = 0;
            *outKnownBytes = false;
            return true;
        }

        if (value.isPointer())
        {
            *outData = value.asPointer();
            *outBytes = 0;
            *outKnownBytes = false;
            return true;
        }

        return false;
    }

    static int native_stbi_failure_reason(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("stbi_failure_reason expects 0 arguments");
            return 0;
        }

        const char *reason = stbi_failure_reason();
        vm->push(vm->makeString(reason ? reason : ""));
        return 1;
    }

    static int native_stbi_set_flip_vertically_on_load(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("stbi_set_flip_vertically_on_load expects 1 numeric argument");
            return 0;
        }
        stbi_set_flip_vertically_on_load(args[0].asInt() ? 1 : 0);
        return 0;
    }

    static int native_stbi_info(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isString())
        {
            Error("stbi_info expects 1 string argument");
            return 0;
        }

        int width = 0;
        int height = 0;
        int channels = 0;

        SDL_RWops *rw = SDL_RWFromFile(args[0].asStringChars(), "rb");
        int ok = 0;
        if (rw)
        {
            ok = stbi_info_from_callbacks(&s_stbi_sdl_callbacks, rw, &width, &height, &channels);
            SDL_RWclose(rw);
        }

        if (!ok)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 3;
        }

        vm->pushInt(width);
        vm->pushInt(height);
        vm->pushInt(channels);

        return 3;
    }

    static int native_stbi_load(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 1 || argc > 2 || !args[0].isString())
        {
            Error("stbi_load expects (path[, desiredChannels])");
            return 0;
        }

        int desiredChannels = 0;
        if (argc == 2)
        {
            if (!args[1].isNumber())
            {
                Error("stbi_load desiredChannels must be numeric");
                return 0;
            }
            desiredChannels = args[1].asInt();
            if (desiredChannels < 0 || desiredChannels > 4)
            {
                Error("stbi_load desiredChannels must be in [0..4]");
                return 0;
            }
        }

        int width = 0;
        int height = 0;
        int sourceChannels = 0;

        SDL_RWops *rw = SDL_RWFromFile(args[0].asStringChars(), "rb");
        stbi_uc *pixels = nullptr;
        if (rw)
        {
            pixels = stbi_load_from_callbacks(&s_stbi_sdl_callbacks, rw, &width, &height, &sourceChannels, desiredChannels);
            SDL_RWclose(rw);
        }

        if (!pixels)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 4;
        }

        const int channels = (desiredChannels > 0) ? desiredChannels : sourceChannels;
        size_t byteCount = 0;
        if (!checked_mul_size((size_t)width, (size_t)height, &byteCount) ||
            !checked_mul_size(byteCount, (size_t)channels, &byteCount) ||
            byteCount > (size_t)std::numeric_limits<int>::max())
        {
            stbi_image_free(pixels);
            Error("stbi_load image too large");
            return 0;
        }

        Value dataValue = vm->makeBuffer((int)byteCount, (int)BufferType::UINT8);
        BufferInstance *buffer = dataValue.asBuffer();
        if (byteCount > 0 && buffer && buffer->data)
        {
            std::memcpy(buffer->data, pixels, byteCount);
        }
        stbi_image_free(pixels);

        vm->pushInt(width);
        vm->pushInt(height);
        vm->pushInt(channels);
        vm->push(dataValue);
        return 4;
    }

    static int native_stbi_load_from_memory(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 1 || argc > 2 || !args[0].isBuffer())
        {
            Error("stbi_load_from_memory expects (buffer[, desiredChannels])");
            return 0;
        }

        int desiredChannels = 0;
        if (argc == 2)
        {
            if (!args[1].isNumber())
            {
                Error("stbi_load_from_memory desiredChannels must be numeric");
                return 0;
            }
            desiredChannels = args[1].asInt();
            if (desiredChannels < 0 || desiredChannels > 4)
            {
                Error("stbi_load_from_memory desiredChannels must be in [0..4]");
                return 0;
            }
        }

        BufferInstance *src = args[0].asBuffer();
        if (!src || !src->data || src->count <= 0 || src->elementSize <= 0)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 4;
        }

        size_t srcBytes = 0;
        if (!checked_mul_size((size_t)src->count, (size_t)src->elementSize, &srcBytes) ||
            srcBytes > (size_t)std::numeric_limits<int>::max())
        {
            Error("stbi_load_from_memory buffer too large");
            return 0;
        }

        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        stbi_uc *pixels = stbi_load_from_memory(src->data, (int)srcBytes, &width, &height, &sourceChannels, desiredChannels);
        if (!pixels)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 4;
        }

        const int channels = (desiredChannels > 0) ? desiredChannels : sourceChannels;
        size_t byteCount = 0;
        if (!checked_mul_size((size_t)width, (size_t)height, &byteCount) ||
            !checked_mul_size(byteCount, (size_t)channels, &byteCount) ||
            byteCount > (size_t)std::numeric_limits<int>::max())
        {
            stbi_image_free(pixels);
            Error("stbi_load_from_memory decoded image too large");
            return 0;
        }

        Value dataValue = vm->makeBuffer((int)byteCount, (int)BufferType::UINT8);
        BufferInstance *buffer = dataValue.asBuffer();
        if (byteCount > 0 && buffer && buffer->data)
        {
            std::memcpy(buffer->data, pixels, byteCount);
        }
        stbi_image_free(pixels);

        vm->pushInt(width);
        vm->pushInt(height);
        vm->pushInt(channels);
        vm->push(dataValue);
        return 4;
    }

    static bool check_min_bytes(size_t knownBytes, bool hasKnownBytes, size_t neededBytes, const char *fnName)
    {
        if (!hasKnownBytes)
            return true;
        if (knownBytes < neededBytes)
        {
            Error("%s data buffer too small: need %zu bytes, got %zu", fnName, neededBytes, knownBytes);
            return false;
        }
        return true;
    }

    static int native_stbi_write_png(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc < 5 || argc > 6 || !args[0].isString() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("stbi_write_png expects (path, width, height, channels, data[, strideBytes])");
            return 0;
        }

        const char *path = args[0].asStringChars();
        int w = args[1].asInt();
        int h = args[2].asInt();
        int comp = args[3].asInt();
        int stride = (w * comp);
        if (argc == 6)
        {
            if (!args[5].isNumber())
            {
                Error("stbi_write_png strideBytes must be numeric");
                return 0;
            }
            stride = args[5].asInt();
        }
        if (w <= 0 || h <= 0 || comp <= 0)
        {
            Error("stbi_write_png width/height/channels must be > 0");
            return 0;
        }
        if (stride <= 0)
        {
            Error("stbi_write_png strideBytes must be > 0");
            return 0;
        }

        const void *data = nullptr;
        size_t dataBytes = 0;
        bool knownBytes = false;
        if (!resolve_image_data_arg(args[4], &data, &dataBytes, &knownBytes))
        {
            Error("stbi_write_png data expects buffer/typedarray/pointer");
            return 0;
        }

        size_t needed = 0;
        if (!checked_mul_size((size_t)w, (size_t)h, &needed) || !checked_mul_size(needed, (size_t)comp, &needed))
        {
            Error("stbi_write_png image size overflow");
            return 0;
        }
        if (!check_min_bytes(dataBytes, knownBytes, needed, "stbi_write_png"))
            return 0;

        int ok = 0;
        SDL_RWops *rw = SDL_RWFromFile(path, "wb");
        if (rw)
        {
            StbiSdlWriteContext ctx = {rw, 0};
            int wrote = stbi_write_png_to_func(stbi_sdl_write, &ctx, w, h, comp, data, stride);
            SDL_RWclose(rw);
            ok = (wrote && !ctx.failed) ? 1 : 0;
        }
        vm->push(vm->makeInt(ok));
        return 1;
    }

    static int native_stbi_write_bmp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5 || !args[0].isString() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("stbi_write_bmp expects (path, width, height, channels, data)");
            return 0;
        }

        const char *path = args[0].asStringChars();
        int w = args[1].asInt();
        int h = args[2].asInt();
        int comp = args[3].asInt();
        if (w <= 0 || h <= 0 || comp <= 0)
        {
            Error("stbi_write_bmp width/height/channels must be > 0");
            return 0;
        }

        const void *data = nullptr;
        size_t dataBytes = 0;
        bool knownBytes = false;
        if (!resolve_image_data_arg(args[4], &data, &dataBytes, &knownBytes))
        {
            Error("stbi_write_bmp data expects buffer/typedarray/pointer");
            return 0;
        }

        size_t needed = 0;
        if (!checked_mul_size((size_t)w, (size_t)h, &needed) || !checked_mul_size(needed, (size_t)comp, &needed))
        {
            Error("stbi_write_bmp image size overflow");
            return 0;
        }
        if (!check_min_bytes(dataBytes, knownBytes, needed, "stbi_write_bmp"))
            return 0;

        int ok = 0;
        SDL_RWops *rw = SDL_RWFromFile(path, "wb");
        if (rw)
        {
            StbiSdlWriteContext ctx = {rw, 0};
            int wrote = stbi_write_bmp_to_func(stbi_sdl_write, &ctx, w, h, comp, data);
            SDL_RWclose(rw);
            ok = (wrote && !ctx.failed) ? 1 : 0;
        }
        vm->push(vm->makeInt(ok));
        return 1;
    }

    static int native_stbi_write_tga(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5 || !args[0].isString() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("stbi_write_tga expects (path, width, height, channels, data)");
            return 0;
        }

        const char *path = args[0].asStringChars();
        int w = args[1].asInt();
        int h = args[2].asInt();
        int comp = args[3].asInt();
        if (w <= 0 || h <= 0 || comp <= 0)
        {
            Error("stbi_write_tga width/height/channels must be > 0");
            return 0;
        }

        const void *data = nullptr;
        size_t dataBytes = 0;
        bool knownBytes = false;
        if (!resolve_image_data_arg(args[4], &data, &dataBytes, &knownBytes))
        {
            Error("stbi_write_tga data expects buffer/typedarray/pointer");
            return 0;
        }

        size_t needed = 0;
        if (!checked_mul_size((size_t)w, (size_t)h, &needed) || !checked_mul_size(needed, (size_t)comp, &needed))
        {
            Error("stbi_write_tga image size overflow");
            return 0;
        }
        if (!check_min_bytes(dataBytes, knownBytes, needed, "stbi_write_tga"))
            return 0;

        int ok = 0;
        SDL_RWops *rw = SDL_RWFromFile(path, "wb");
        if (rw)
        {
            StbiSdlWriteContext ctx = {rw, 0};
            int wrote = stbi_write_tga_to_func(stbi_sdl_write, &ctx, w, h, comp, data);
            SDL_RWclose(rw);
            ok = (wrote && !ctx.failed) ? 1 : 0;
        }
        vm->push(vm->makeInt(ok));
        return 1;
    }

    static int native_stbi_write_hdr(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5 || !args[0].isString() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("stbi_write_hdr expects (path, width, height, channels, floatData)");
            return 0;
        }

        const char *path = args[0].asStringChars();
        int w = args[1].asInt();
        int h = args[2].asInt();
        int comp = args[3].asInt();
        if (w <= 0 || h <= 0 || comp <= 0)
        {
            Error("stbi_write_hdr width/height/channels must be > 0");
            return 0;
        }

        const void *raw = nullptr;
        size_t dataBytes = 0;
        bool knownBytes = false;
        if (!resolve_image_data_arg(args[4], &raw, &dataBytes, &knownBytes))
        {
            Error("stbi_write_hdr data expects buffer/typedarray/pointer");
            return 0;
        }

        size_t needed = 0;
        if (!checked_mul_size((size_t)w, (size_t)h, &needed) ||
            !checked_mul_size(needed, (size_t)comp, &needed) ||
            !checked_mul_size(needed, sizeof(float), &needed))
        {
            Error("stbi_write_hdr image size overflow");
            return 0;
        }
        if (!check_min_bytes(dataBytes, knownBytes, needed, "stbi_write_hdr"))
            return 0;

        int ok = 0;
        SDL_RWops *rw = SDL_RWFromFile(path, "wb");
        if (rw)
        {
            StbiSdlWriteContext ctx = {rw, 0};
            int wrote = stbi_write_hdr_to_func(stbi_sdl_write, &ctx, w, h, comp, (const float *)raw);
            SDL_RWclose(rw);
            ok = (wrote && !ctx.failed) ? 1 : 0;
        }
        vm->push(vm->makeInt(ok));
        return 1;
    }

    static int native_stbir_resize_uint8_linear(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 6 || argc > 8 || !args[1].isNumber() || !args[2].isNumber() ||
            !args[3].isNumber() || !args[4].isNumber() || !args[5].isNumber())
        {
            Error("stbir_resize_uint8_linear expects (src, inW, inH, channels, outW, outH[, inStrideBytes, outStrideBytes])");
            return 0;
        }

        const void *src = nullptr;
        size_t srcBytes = 0;
        bool srcKnownBytes = false;
        if (!resolve_image_data_arg(args[0], &src, &srcBytes, &srcKnownBytes))
        {
            Error("stbir_resize_uint8_linear src expects buffer/typedarray/pointer");
            return 0;
        }
        if (!src)
        {
            Error("stbir_resize_uint8_linear src is null");
            return 0;
        }

        const int inW = args[1].asInt();
        const int inH = args[2].asInt();
        const int comp = args[3].asInt();
        const int outW = args[4].asInt();
        const int outH = args[5].asInt();
        if (inW <= 0 || inH <= 0 || outW <= 0 || outH <= 0)
        {
            Error("stbir_resize_uint8_linear dimensions must be > 0");
            return 0;
        }
        if (comp < 1 || comp > 4)
        {
            Error("stbir_resize_uint8_linear channels must be in [1..4]");
            return 0;
        }

        int inStride = inW * comp;
        int outStride = outW * comp;
        if (argc >= 7)
        {
            if (!args[6].isNumber())
            {
                Error("stbir_resize_uint8_linear inStrideBytes must be numeric");
                return 0;
            }
            inStride = args[6].asInt();
        }
        if (argc == 8)
        {
            if (!args[7].isNumber())
            {
                Error("stbir_resize_uint8_linear outStrideBytes must be numeric");
                return 0;
            }
            outStride = args[7].asInt();
        }

        if (inStride <= 0 || outStride <= 0)
        {
            Error("stbir_resize_uint8_linear strides must be > 0");
            return 0;
        }

        size_t neededSrc = 0;
        size_t neededDst = 0;
        if (!checked_mul_size((size_t)inStride, (size_t)inH, &neededSrc) ||
            !checked_mul_size((size_t)outStride, (size_t)outH, &neededDst))
        {
            Error("stbir_resize_uint8_linear size overflow");
            return 0;
        }
        if (srcKnownBytes && srcBytes < neededSrc)
        {
            Error("stbir_resize_uint8_linear src too small: need %zu bytes, got %zu", neededSrc, srcBytes);
            return 0;
        }
        if (neededDst > (size_t)std::numeric_limits<int>::max())
        {
            Error("stbir_resize_uint8_linear output too large");
            return 0;
        }

        Value out = vm->makeBuffer((int)neededDst, (int)BufferType::UINT8);
        BufferInstance *dst = out.asBuffer();
        if (!dst || !dst->data)
        {
            Error("stbir_resize_uint8_linear failed to allocate output buffer");
            return 0;
        }

        unsigned char *result = stbir_resize_uint8_linear(
            (const unsigned char *)src, inW, inH, inStride,
            dst->data, outW, outH, outStride,
            (stbir_pixel_layout)comp);

        if (!result)
        {
            vm->push(vm->makeNil());
            return 1;
        }

        vm->push(out);
        return 1;
    }

    static int native_stb_perlin_noise3(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("stb_perlin_noise3 expects (x, y, z, x_wrap, y_wrap, z_wrap)");
            return 0;
        }

        const float v = stb_perlin_noise3((float)args[0].asNumber(),
                                          (float)args[1].asNumber(),
                                          (float)args[2].asNumber(),
                                          args[3].asInt(),
                                          args[4].asInt(),
                                          args[5].asInt());
        vm->pushDouble(v);
        return 1;
    }

    static int native_stb_perlin_noise3_seed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 7)
        {
            Error("stb_perlin_noise3_seed expects (x, y, z, x_wrap, y_wrap, z_wrap, seed)");
            return 0;
        }

        const float v = stb_perlin_noise3_seed((float)args[0].asNumber(),
                                               (float)args[1].asNumber(),
                                               (float)args[2].asNumber(),
                                               args[3].asInt(),
                                               args[4].asInt(),
                                               args[5].asInt(),
                                               args[6].asInt());
        vm->pushDouble(v);
        return 1;
    }

    static int native_stb_perlin_noise3_wrap_nonpow2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 7)
        {
            Error("stb_perlin_noise3_wrap_nonpow2 expects (x, y, z, x_wrap, y_wrap, z_wrap, seedByte)");
            return 0;
        }

        int seed = args[6].asInt();
        if (seed < 0)
            seed = 0;
        if (seed > 255)
            seed = 255;

        const float v = stb_perlin_noise3_wrap_nonpow2((float)args[0].asNumber(),
                                                       (float)args[1].asNumber(),
                                                       (float)args[2].asNumber(),
                                                       args[3].asInt(),
                                                       args[4].asInt(),
                                                       args[5].asInt(),
                                                       (unsigned char)seed);
        vm->pushDouble(v);
        return 1;
    }

    static int native_stb_perlin_ridge_noise3(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 7)
        {
            Error("stb_perlin_ridge_noise3 expects (x, y, z, lacunarity, gain, offset, octaves)");
            return 0;
        }

        const float v = stb_perlin_ridge_noise3((float)args[0].asNumber(),
                                                (float)args[1].asNumber(),
                                                (float)args[2].asNumber(),
                                                (float)args[3].asNumber(),
                                                (float)args[4].asNumber(),
                                                (float)args[5].asNumber(),
                                                args[6].asInt());
        vm->pushDouble(v);
        return 1;
    }

    static int native_stb_perlin_fbm_noise3(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("stb_perlin_fbm_noise3 expects (x, y, z, lacunarity, gain, octaves)");
            return 0;
        }

        const float v = stb_perlin_fbm_noise3((float)args[0].asNumber(),
                                              (float)args[1].asNumber(),
                                              (float)args[2].asNumber(),
                                              (float)args[3].asNumber(),
                                              (float)args[4].asNumber(),
                                              args[5].asInt());
        vm->pushDouble(v);
        return 1;
    }

    static int native_stb_perlin_turbulence_noise3(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("stb_perlin_turbulence_noise3 expects (x, y, z, lacunarity, gain, octaves)");
            return 0;
        }

        const float v = stb_perlin_turbulence_noise3((float)args[0].asNumber(),
                                                     (float)args[1].asNumber(),
                                                     (float)args[2].asNumber(),
                                                     (float)args[3].asNumber(),
                                                     (float)args[4].asNumber(),
                                                     args[5].asInt());
        vm->pushDouble(v);
        return 1;
    }

    void register_stb(ModuleBuilder &module)
    {
        module.addFunction("stbi_failure_reason", native_stbi_failure_reason, 0)
            .addFunction("stbi_set_flip_vertically_on_load", native_stbi_set_flip_vertically_on_load, 1)
            .addFunction("stbi_info", native_stbi_info, 1)
            .addFunction("stbi_load", native_stbi_load, -1)
            .addFunction("stbi_load_from_memory", native_stbi_load_from_memory, -1)
            .addFunction("stbi_write_png", native_stbi_write_png, -1)
            .addFunction("stbi_write_bmp", native_stbi_write_bmp, 5)
            .addFunction("stbi_write_tga", native_stbi_write_tga, 5)
            .addFunction("stbi_write_hdr", native_stbi_write_hdr, 5)
            .addFunction("stbir_resize_uint8_linear", native_stbir_resize_uint8_linear, -1)
            .addFunction("stb_perlin_noise3", native_stb_perlin_noise3, 6)
            .addFunction("stb_perlin_noise3_seed", native_stb_perlin_noise3_seed, 7)
            .addFunction("stb_perlin_noise3_wrap_nonpow2", native_stb_perlin_noise3_wrap_nonpow2, 7)
            .addFunction("stb_perlin_ridge_noise3", native_stb_perlin_ridge_noise3, 7)
            .addFunction("stb_perlin_fbm_noise3", native_stb_perlin_fbm_noise3, 6)
            .addFunction("stb_perlin_turbulence_noise3", native_stb_perlin_turbulence_noise3, 6)
            .addInt("STBI_default", STBI_default)
            .addInt("STBI_grey", STBI_grey)
            .addInt("STBI_grey_alpha", STBI_grey_alpha)
            .addInt("STBI_rgb", STBI_rgb)
            .addInt("STBI_rgb_alpha", STBI_rgb_alpha);
    }
}
