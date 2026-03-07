#include "bindings.hpp"
#define MSF_GIF_IMPL
#include "msf_gif.h"

#include <cstring>
#include <limits>

namespace SDLBindings
{
    static constexpr const char *kGifClassName = "Gif";

    struct GifEncoderData
    {
        MsfGifState state;
        int width;
        int height;
        int active;
    };

    static bool checked_mul_size(size_t a, size_t b, size_t *out)
    {
        if (!out)
            return false;
        if (a != 0 && b > (std::numeric_limits<size_t>::max() / a))
            return false;
        *out = a * b;
        return true;
    }

    static bool resolve_gif_pixels_arg(const Value &value, const void **outData, size_t *outBytes, bool *outKnownBytes)
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

    static GifEncoderData *as_gif(void *instance)
    {
        return (GifEncoderData *)instance;
    }

    static void gif_discard_state(GifEncoderData *gif)
    {
        if (!gif)
            return;

        if (gif->active)
        {
            MsfGifResult result = msf_gif_end(&gif->state);
            if (result.data)
                msf_gif_free(result);
        }

        gif->state = {};
        gif->width = 0;
        gif->height = 0;
        gif->active = 0;
    }

    static void *gif_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("Gif expects 0 arguments");
            return nullptr;
        }

        GifEncoderData *gif = new GifEncoderData();
        gif->state = {};
        gif->width = 0;
        gif->height = 0;
        gif->active = 0;
        return gif;
    }

    static void gif_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        GifEncoderData *gif = as_gif(instance);
        if (!gif)
            return;
        gif_discard_state(gif);
        delete gif;
    }

    static int gif_begin(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        GifEncoderData *gif = as_gif(instance);
        if (!gif || argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Gif.begin() expects (width, height)");
            return 0;
        }

        const int width = args[0].asInt();
        const int height = args[1].asInt();
        if (width <= 0 || height <= 0)
        {
            Error("Gif.begin() width/height must be > 0");
            return 0;
        }

        gif_discard_state(gif);
        const int ok = msf_gif_begin(&gif->state, width, height) ? 1 : 0;
        if (ok)
        {
            gif->width = width;
            gif->height = height;
            gif->active = 1;
        }
        vm->pushInt(ok);
        return 1;
    }

    static int gif_frame(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        GifEncoderData *gif = as_gif(instance);
        if (!gif || (argCount != 3 && argCount != 4))
        {
            Error("Gif.frame() expects (rgbaPixels, centiseconds, maxBitDepth[, pitchBytes])");
            return 0;
        }
        if (!gif->active)
        {
            Error("Gif.frame() called before begin()");
            return 0;
        }

        const void *pixelsRaw = nullptr;
        size_t pixelsBytes = 0;
        bool pixelsKnownBytes = false;
        if (!resolve_gif_pixels_arg(args[0], &pixelsRaw, &pixelsBytes, &pixelsKnownBytes))
        {
            Error("Gif.frame() rgbaPixels expects buffer/typedarray/pointer");
            return 0;
        }
        if (!pixelsRaw)
        {
            Error("Gif.frame() rgbaPixels is null");
            return 0;
        }

        const int centis = args[1].asInt();
        const int maxBitDepth = args[2].asInt();
        const int pitch = (argCount == 4) ? args[3].asInt() : (gif->width * 4);
        if (pitch == 0)
        {
            Error("Gif.frame() pitchBytes must be != 0");
            return 0;
        }

        const unsigned long long pitchAbs = (pitch < 0)
                                                ? (unsigned long long)(-(long long)pitch)
                                                : (unsigned long long)pitch;
        if (pitchAbs == 0 || pitchAbs > (unsigned long long)std::numeric_limits<size_t>::max())
        {
            Error("Gif.frame() invalid pitchBytes");
            return 0;
        }

        size_t needed = 0;
        if (!checked_mul_size((size_t)pitchAbs, (size_t)gif->height, &needed))
        {
            Error("Gif.frame() size overflow");
            return 0;
        }
        if (pixelsKnownBytes && pixelsBytes < needed)
        {
            Error("Gif.frame() rgbaPixels too small: need %zu bytes, got %zu", needed, pixelsBytes);
            return 0;
        }

        const int ok = msf_gif_frame(&gif->state, (uint8_t *)pixelsRaw, centis, maxBitDepth, pitch) ? 1 : 0;
        vm->pushInt(ok);
        return 1;
    }

    static int gif_end(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        GifEncoderData *gif = as_gif(instance);
        if (!gif || argCount != 0)
        {
            Error("Gif.end() expects 0 arguments");
            return 0;
        }

        if (!gif->active)
        {
            vm->push(vm->makeNil());
            return 1;
        }

        MsfGifResult result = msf_gif_end(&gif->state);
        gif->state = {};
        gif->width = 0;
        gif->height = 0;
        gif->active = 0;

        if (!result.data || result.dataSize == 0)
        {
            if (result.data)
                msf_gif_free(result);
            vm->push(vm->makeNil());
            return 1;
        }

        if (result.dataSize > (size_t)std::numeric_limits<int>::max())
        {
            msf_gif_free(result);
            Error("Gif.end() result too large");
            return 0;
        }

        Value out = vm->makeBuffer((int)result.dataSize, (int)BufferType::UINT8);
        BufferInstance *buf = out.asBuffer();
        if (buf && buf->data && result.dataSize > 0)
            std::memcpy(buf->data, result.data, result.dataSize);
        msf_gif_free(result);

        vm->push(out);
        return 1;
    }

    static int gif_save(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        GifEncoderData *gif = as_gif(instance);
        if (!gif || argCount != 1 || !args[0].isString())
        {
            Error("Gif.save() expects (path)");
            return 0;
        }
        if (!gif->active)
        {
            vm->pushInt(0);
            return 1;
        }

        const char *path = args[0].asStringChars();
        MsfGifResult result = msf_gif_end(&gif->state);
        gif->state = {};
        gif->width = 0;
        gif->height = 0;
        gif->active = 0;

        if (!result.data || result.dataSize == 0)
        {
            if (result.data)
                msf_gif_free(result);
            vm->pushInt(0);
            return 1;
        }

        int ok = 0;
        SDL_RWops *rw = SDL_RWFromFile(path, "wb");
        if (rw)
        {
            const size_t chunksWritten = SDL_RWwrite(rw, result.data, result.dataSize, 1);
            SDL_RWclose(rw);
            ok = (chunksWritten == 1) ? 1 : 0;
        }
        msf_gif_free(result);

        vm->pushInt(ok);
        return 1;
    }

    static int gif_set_options(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)instance;
        if (argCount != 2)
        {
            Error("Gif.setOptions() expects (alphaThreshold, bgraFlag)");
            return 0;
        }

        int alpha = args[0].asInt();
        if (alpha < 0)
            alpha = 0;
        if (alpha > 255)
            alpha = 255;

        msf_gif_alpha_threshold = alpha;
        msf_gif_bgra_flag = args[1].asBool() ? 1 : 0;
        return 0;
    }

    static int gif_reset(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        GifEncoderData *gif = as_gif(instance);
        if (!gif || argCount != 0)
        {
            Error("Gif.reset() expects 0 arguments");
            return 0;
        }
        gif_discard_state(gif);
        vm->pushInt(1);
        return 1;
    }

    static Value gif_get_width(Interpreter *vm, void *instance)
    {
        GifEncoderData *gif = as_gif(instance);
        return vm->makeInt(gif ? gif->width : 0);
    }

    static Value gif_get_height(Interpreter *vm, void *instance)
    {
        GifEncoderData *gif = as_gif(instance);
        return vm->makeInt(gif ? gif->height : 0);
    }

    static Value gif_get_active(Interpreter *vm, void *instance)
    {
        GifEncoderData *gif = as_gif(instance);
        return vm->makeInt((gif && gif->active) ? 1 : 0);
    }

    void register_msf_gif(Interpreter &vm)
    {
        NativeClassDef *gifClass = vm.registerNativeClass(kGifClassName, gif_ctor, gif_dtor, 0, false);
        vm.addNativeMethod(gifClass, "begin", gif_begin);
        vm.addNativeMethod(gifClass, "frame", gif_frame);
        vm.addNativeMethod(gifClass, "addFrame", gif_frame);
        vm.addNativeMethod(gifClass, "end", gif_end);
        vm.addNativeMethod(gifClass, "save", gif_save);
        vm.addNativeMethod(gifClass, "setOptions", gif_set_options);
        vm.addNativeMethod(gifClass, "reset", gif_reset);
        vm.addNativeProperty(gifClass, "width", gif_get_width, nullptr);
        vm.addNativeProperty(gifClass, "height", gif_get_height, nullptr);
        vm.addNativeProperty(gifClass, "active", gif_get_active, nullptr);
    }
}
