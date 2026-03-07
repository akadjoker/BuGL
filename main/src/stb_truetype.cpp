#include "bindings.hpp"
 #define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <cstring>
#include <limits>
#include <vector>

namespace SDLBindings
{
    static constexpr const char *kClassFont = "Font";

    struct FontData
    {
        std::vector<unsigned char> bytes;
        stbtt_fontinfo info;
        int fontOffset;
        int loaded;
        std::vector<stbtt_bakedchar> baked;
        int bakedWidth;
        int bakedHeight;
        int bakedFirstChar;
        int bakedNumChars;
    };

    static FontData *as_font(void *instance)
    {
        return (FontData *)instance;
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

    static bool read_file_bytes_sdl(const char *path, std::vector<unsigned char> *outBytes)
    {
        if (!path || !outBytes)
            return false;

        SDL_RWops *rw = SDL_RWFromFile(path, "rb");
        if (!rw)
            return false;

        outBytes->clear();
        const Sint64 rawSize = SDL_RWsize(rw);
        if (rawSize > 0)
        {
            const size_t size = (size_t)rawSize;
            outBytes->resize(size);
            const size_t got = SDL_RWread(rw, outBytes->data(), 1, size);
            SDL_RWclose(rw);
            if (got != size)
            {
                outBytes->clear();
                return false;
            }
            return true;
        }

        unsigned char chunk[4096];
        for (;;)
        {
            const size_t got = SDL_RWread(rw, chunk, 1, sizeof(chunk));
            if (got > 0)
                outBytes->insert(outBytes->end(), chunk, chunk + got);
            if (got < sizeof(chunk))
                break;
        }
        SDL_RWclose(rw);
        return !outBytes->empty();
    }

    static bool copy_buffer_bytes(const Value &value, std::vector<unsigned char> *outBytes)
    {
        if (!outBytes || !value.isBuffer())
            return false;

        BufferInstance *buf = value.asBuffer();
        if (!buf || !buf->data || buf->count <= 0 || buf->elementSize <= 0)
        {
            outBytes->clear();
            return false;
        }

        size_t totalBytes = 0;
        if (!checked_mul_size((size_t)buf->count, (size_t)buf->elementSize, &totalBytes))
            return false;

        outBytes->resize(totalBytes);
        std::memcpy(outBytes->data(), buf->data, totalBytes);
        return true;
    }

    static bool init_font(FontData *font, int fontIndex)
    {
        if (!font || font->bytes.empty())
            return false;

        if (fontIndex < 0)
            return false;

        const int offset = stbtt_GetFontOffsetForIndex(font->bytes.data(), fontIndex);
        if (offset < 0)
            return false;

        if (!stbtt_InitFont(&font->info, font->bytes.data(), offset))
            return false;

        font->fontOffset = offset;
        font->loaded = 1;
        font->baked.clear();
        font->bakedWidth = 0;
        font->bakedHeight = 0;
        font->bakedFirstChar = 0;
        font->bakedNumChars = 0;
        return true;
    }

    static void clear_font(FontData *font)
    {
        if (!font)
            return;
        font->bytes.clear();
        font->baked.clear();
        font->loaded = 0;
        font->fontOffset = 0;
        font->bakedWidth = 0;
        font->bakedHeight = 0;
        font->bakedFirstChar = 0;
        font->bakedNumChars = 0;
    }

    static void *font_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        FontData *font = new FontData();
        std::memset(&font->info, 0, sizeof(font->info));
        font->fontOffset = 0;
        font->loaded = 0;
        font->bakedWidth = 0;
        font->bakedHeight = 0;
        font->bakedFirstChar = 0;
        font->bakedNumChars = 0;

        if (argCount == 0)
            return font;

        int fontIndex = 0;
        if (argCount == 2)
        {
            if (!args[1].isNumber())
            {
                Error("Font(source[, index]) index must be numeric");
                delete font;
                return nullptr;
            }
            fontIndex = args[1].asInt();
        }
        else if (argCount != 1)
        {
            Error("Font expects 0, 1 or 2 arguments: source[, index]");
            delete font;
            return nullptr;
        }

        bool ok = false;
        if (args[0].isString())
            ok = read_file_bytes_sdl(args[0].asStringChars(), &font->bytes);
        else if (args[0].isBuffer())
            ok = copy_buffer_bytes(args[0], &font->bytes);
        else
        {
            Error("Font source expects string path or buffer");
            delete font;
            return nullptr;
        }

        if (!ok || !init_font(font, fontIndex))
        {
            Error("Font failed to load source");
            delete font;
            return nullptr;
        }

        return font;
    }

    static void font_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        FontData *font = as_font(instance);
        if (!font)
            return;
        clear_font(font);
        delete font;
    }

    static int font_load(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        FontData *font = as_font(instance);
        if (!font || argCount < 1 || argCount > 2)
        {
            Error("Font.load() expects (source[, index])");
            return 0;
        }

        int fontIndex = 0;
        if (argCount == 2)
        {
            if (!args[1].isNumber())
            {
                Error("Font.load() index must be numeric");
                return 0;
            }
            fontIndex = args[1].asInt();
        }

        std::vector<unsigned char> newBytes;
        bool ok = false;
        if (args[0].isString())
            ok = read_file_bytes_sdl(args[0].asStringChars(), &newBytes);
        else if (args[0].isBuffer())
            ok = copy_buffer_bytes(args[0], &newBytes);
        else
        {
            Error("Font.load() source expects string path or buffer");
            return 0;
        }

        if (!ok)
        {
            vm->pushInt(0);
            return 1;
        }

        font->bytes.swap(newBytes);
        if (!init_font(font, fontIndex))
        {
            clear_font(font);
            vm->pushInt(0);
            return 1;
        }

        vm->pushInt(1);
        return 1;
    }

    static int font_clear(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        FontData *font = as_font(instance);
        if (!font || argCount != 0)
        {
            Error("Font.clear() expects 0 arguments");
            return 0;
        }
        clear_font(font);
        vm->pushInt(1);
        return 1;
    }

    static int font_scale(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        FontData *font = as_font(instance);
        if (!font || argCount != 1 || !args[0].isNumber())
        {
            Error("Font.scaleForPixelHeight() expects (pixelHeight)");
            return 0;
        }
        if (!font->loaded)
        {
            vm->push(vm->makeNil());
            return 1;
        }
        const float scale = stbtt_ScaleForPixelHeight(&font->info, (float)args[0].asNumber());
        vm->pushDouble(scale);
        return 1;
    }

    static int font_metrics(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        FontData *font = as_font(instance);
        if (!font || argCount != 1 || !args[0].isNumber())
        {
            Error("Font.metrics() expects (pixelHeight)");
            return 0;
        }
        if (!font->loaded)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 4;
        }

        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &lineGap);

        const float scale = stbtt_ScaleForPixelHeight(&font->info, (float)args[0].asNumber());
        vm->pushDouble((double)ascent * (double)scale);
        vm->pushDouble((double)descent * (double)scale);
        vm->pushDouble((double)lineGap * (double)scale);
        vm->pushDouble(scale);
        return 4;
    }

    static int font_hmetrics(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        FontData *font = as_font(instance);
        if (!font || argCount < 1 || argCount > 2 || !args[0].isNumber())
        {
            Error("Font.hmetrics() expects (codepoint[, pixelHeight])");
            return 0;
        }
        if (!font->loaded)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 3;
        }

        int advance = 0;
        int lsb = 0;
        const int codepoint = args[0].asInt();
        stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &lsb);

        float scale = 1.0f;
        if (argCount == 2)
        {
            if (!args[1].isNumber())
            {
                Error("Font.hmetrics() pixelHeight must be numeric");
                return 0;
            }
            scale = stbtt_ScaleForPixelHeight(&font->info, (float)args[1].asNumber());
        }

        vm->pushInt(advance);
        vm->pushInt(lsb);
        vm->pushDouble((double)advance * (double)scale);
        return 3;
    }

    static int font_kerning(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        FontData *font = as_font(instance);
        if (!font || argCount < 2 || argCount > 3 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("Font.kerning() expects (codepointA, codepointB[, pixelHeight])");
            return 0;
        }
        if (!font->loaded)
        {
            vm->push(vm->makeNil());
            return 1;
        }

        int k = stbtt_GetCodepointKernAdvance(&font->info, args[0].asInt(), args[1].asInt());
        if (argCount == 3)
        {
            if (!args[2].isNumber())
            {
                Error("Font.kerning() pixelHeight must be numeric");
                return 0;
            }
            const float scale = stbtt_ScaleForPixelHeight(&font->info, (float)args[2].asNumber());
            vm->pushDouble((double)k * (double)scale);
            return 1;
        }

        vm->pushInt(k);
        return 1;
    }

    static int font_bake(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        FontData *font = as_font(instance);
        if (!font || argCount < 3 || argCount > 5 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("Font.bake() expects (pixelHeight, atlasWidth, atlasHeight[, firstChar, numChars])");
            return 0;
        }
        if (!font->loaded)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 5;
        }

        const float pixelHeight = (float)args[0].asNumber();
        const int atlasW = args[1].asInt();
        const int atlasH = args[2].asInt();
        int firstChar = 32;
        int numChars = 96;
        if (argCount >= 4)
            firstChar = args[3].asInt();
        if (argCount >= 5)
            numChars = args[4].asInt();

        if (pixelHeight <= 0.0f || atlasW <= 0 || atlasH <= 0 || numChars <= 0)
        {
            Error("Font.bake() invalid dimensions/size");
            return 0;
        }

        size_t pixelsBytes = 0;
        if (!checked_mul_size((size_t)atlasW, (size_t)atlasH, &pixelsBytes) || pixelsBytes > (size_t)std::numeric_limits<int>::max())
        {
            Error("Font.bake() atlas too large");
            return 0;
        }

        Value bitmap = vm->makeBuffer((int)pixelsBytes, (int)BufferType::UINT8);
        BufferInstance *bmp = bitmap.asBuffer();
        if (!bmp || !bmp->data)
        {
            Error("Font.bake() failed to allocate bitmap");
            return 0;
        }
        std::memset(bmp->data, 0, pixelsBytes);
        // Keep bitmap rooted while we allocate/fill other VM values.
        vm->push(bitmap);

        font->baked.resize((size_t)numChars);
        int row = stbtt_BakeFontBitmap(font->bytes.data(),
                                       font->fontOffset,
                                       pixelHeight,
                                       bmp->data,
                                       atlasW,
                                       atlasH,
                                       firstChar,
                                       numChars,
                                       font->baked.data());

        font->bakedWidth = atlasW;
        font->bakedHeight = atlasH;
        font->bakedFirstChar = firstChar;
        font->bakedNumChars = numChars;

        Value glyphs = vm->makeArray();
        // Keep glyph array rooted while we push many values into it.
        vm->push(glyphs);
        ArrayInstance *arr = glyphs.asArray();
        arr->values.reserve((size_t)numChars * 7u);
        for (int i = 0; i < numChars; i++)
        {
            const stbtt_bakedchar &g = font->baked[(size_t)i];
            arr->values.push(vm->makeInt((int)g.x0));
            arr->values.push(vm->makeInt((int)g.y0));
            arr->values.push(vm->makeInt((int)g.x1));
            arr->values.push(vm->makeInt((int)g.y1));
            arr->values.push(vm->makeDouble(g.xoff));
            arr->values.push(vm->makeDouble(g.yoff));
            arr->values.push(vm->makeDouble(g.xadvance));
        }

        Value glyphsOut = vm->pop();
        Value bitmapOut = vm->pop();

        vm->pushInt(row);
        vm->pushInt(firstChar);
        vm->pushInt(numChars);
        vm->push(glyphsOut);
        vm->push(bitmapOut);
        return 5;
    }

    static int font_quad(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        FontData *font = as_font(instance);
        if (!font || argCount < 3 || argCount > 4 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("Font.getBakedQuad() expects (codepoint, x, y[, openglFillRule])");
            return 0;
        }

        if (font->baked.empty() || font->bakedNumChars <= 0)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 3;
        }

        const int charIndex = args[0].asInt() - font->bakedFirstChar;
        if (charIndex < 0 || charIndex >= font->bakedNumChars)
        {
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            vm->push(vm->makeNil());
            return 3;
        }

        float x = (float)args[1].asNumber();
        float y = (float)args[2].asNumber();
        int openglFillRule = 1;
        if (argCount == 4)
            openglFillRule = args[3].asInt() ? 1 : 0;

        stbtt_aligned_quad q = {};
        stbtt_GetBakedQuad(font->baked.data(), font->bakedWidth, font->bakedHeight, charIndex, &x, &y, &q, openglFillRule);

        Value quad = vm->makeArray();
        ArrayInstance *arr = quad.asArray();
        arr->values.reserve(8);
        arr->values.push(vm->makeDouble(q.x0));
        arr->values.push(vm->makeDouble(q.y0));
        arr->values.push(vm->makeDouble(q.s0));
        arr->values.push(vm->makeDouble(q.t0));
        arr->values.push(vm->makeDouble(q.x1));
        arr->values.push(vm->makeDouble(q.y1));
        arr->values.push(vm->makeDouble(q.s1));
        arr->values.push(vm->makeDouble(q.t1));

        vm->pushDouble(x);
        vm->pushDouble(y);
        vm->push(quad);
        return 3;
    }

    static Value font_get_loaded(Interpreter *vm, void *instance)
    {
        FontData *font = as_font(instance);
        return vm->makeInt((font && font->loaded) ? 1 : 0);
    }

    static Value font_get_byte_length(Interpreter *vm, void *instance)
    {
        FontData *font = as_font(instance);
        if (!font)
            return vm->makeInt(0);
        if (font->bytes.size() > (size_t)std::numeric_limits<int>::max())
            return vm->makeInt(0);
        return vm->makeInt((int)font->bytes.size());
    }

    static Value font_get_baked_count(Interpreter *vm, void *instance)
    {
        FontData *font = as_font(instance);
        return vm->makeInt(font ? font->bakedNumChars : 0);
    }

    static int native_stbtt_get_number_of_fonts(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isBuffer())
        {
            Error("stbtt_GetNumberOfFonts expects (buffer)");
            return 0;
        }
        BufferInstance *buf = args[0].asBuffer();
        if (!buf || !buf->data || buf->count <= 0 || buf->elementSize <= 0)
        {
            vm->push(vm->makeNil());
            return 1;
        }
        size_t bytes = 0;
        if (!checked_mul_size((size_t)buf->count, (size_t)buf->elementSize, &bytes) || bytes > (size_t)std::numeric_limits<int>::max())
        {
            Error("stbtt_GetNumberOfFonts buffer too large");
            return 0;
        }
        vm->pushInt(stbtt_GetNumberOfFonts(buf->data));
        return 1;
    }

    void register_stb_truetype(ModuleBuilder &module, Interpreter &vm)
    {
        NativeClassDef *fontClass = vm.registerNativeClass(kClassFont, font_ctor, font_dtor, -1, false);
        vm.addNativeMethod(fontClass, "load", font_load);
        vm.addNativeMethod(fontClass, "clear", font_clear);
        vm.addNativeMethod(fontClass, "scaleForPixelHeight", font_scale);
        vm.addNativeMethod(fontClass, "metrics", font_metrics);
        vm.addNativeMethod(fontClass, "hmetrics", font_hmetrics);
        vm.addNativeMethod(fontClass, "kerning", font_kerning);
        vm.addNativeMethod(fontClass, "bake", font_bake);
        vm.addNativeMethod(fontClass, "getBakedQuad", font_quad);
        vm.addNativeProperty(fontClass, "loaded", font_get_loaded, nullptr);
        vm.addNativeProperty(fontClass, "byteLength", font_get_byte_length, nullptr);
        vm.addNativeProperty(fontClass, "bakedCount", font_get_baked_count, nullptr);

        module.addFunction("stbtt_GetNumberOfFonts", native_stbtt_get_number_of_fonts, 1);
    }
}
