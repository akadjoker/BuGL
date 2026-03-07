#include "bindings.hpp"

namespace SDLBindings
{
    int native_SDL_CreateRenderer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_CreateRenderer expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_CreateRenderer expects window pointer");
            return 0;
        }

        SDL_Window *window = (SDL_Window *)args[0].asPointer();
        int index = args[1].asNumber();
        Uint32 flags = args[2].asNumber();

        SDL_Renderer *renderer = SDL_CreateRenderer(window, index, flags);
        vm->push(vm->makePointer(renderer));
        return 1;
    }

    int native_SDL_DestroyRenderer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_DestroyRenderer expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_DestroyRenderer expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_DestroyRenderer(renderer);
        return 0;
    }

    int native_SDL_SetRenderDrawColor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_SetRenderDrawColor expects 5 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetRenderDrawColor expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        Uint8 r = args[1].asNumber();
        Uint8 g = args[2].asNumber();
        Uint8 b = args[3].asNumber();
        Uint8 a = args[4].asNumber();

        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        return 0;
    }

    int native_SDL_RenderClear(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RenderClear expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderClear expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_RenderClear(renderer);
        return 0;
    }

    int native_SDL_RenderPresent(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RenderPresent expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderPresent expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_RenderPresent(renderer);
        return 0;
    }

    int native_SDL_RenderDrawPoint(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_RenderDrawPoint expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderDrawPoint expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        int x = args[1].asNumber();
        int y = args[2].asNumber();

        SDL_RenderDrawPoint(renderer, x, y);
        return 0;
    }

    int native_SDL_RenderDrawLine(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_RenderDrawLine expects 5 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderDrawLine expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        int x1 = args[1].asNumber();
        int y1 = args[2].asNumber();
        int x2 = args[3].asNumber();
        int y2 = args[4].asNumber();

        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        return 0;
    }

    int native_SDL_RenderDrawRect(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_RenderDrawRect expects 5 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderDrawRect expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Rect rect;
        rect.x = args[1].asNumber();
        rect.y = args[2].asNumber();
        rect.w = args[3].asNumber();
        rect.h = args[4].asNumber();

        SDL_RenderDrawRect(renderer, &rect);
        return 0;
    }

    int native_SDL_RenderFillRect(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_RenderFillRect expects 5 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderFillRect expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Rect rect;
        rect.x = args[1].asNumber();
        rect.y = args[2].asNumber();
        rect.w = args[3].asNumber();
        rect.h = args[4].asNumber();

        SDL_RenderFillRect(renderer, &rect);
        return 0;
    }

    int native_SDL_SetRenderDrawBlendMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetRenderDrawBlendMode expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetRenderDrawBlendMode expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_BlendMode blendMode = (SDL_BlendMode)args[1].asNumber();

        SDL_SetRenderDrawBlendMode(renderer, blendMode);
        return 0;
    }

    int native_SDL_RenderSetScale(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_RenderSetScale expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderSetScale expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        float scaleX = args[1].asNumber();
        float scaleY = args[2].asNumber();

        SDL_RenderSetScale(renderer, scaleX, scaleY);
        return 0;
    }

    int native_SDL_RenderGetScale(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RenderGetScale expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderGetScale expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        float scaleX, scaleY;
        SDL_RenderGetScale(renderer, &scaleX, &scaleY);

        vm->pushDouble(scaleX);
        vm->pushDouble(scaleY);
        return 2;
        
    }

    // =====================================================
    // RENDERER EXTRA FUNCTIONS
    // =====================================================

    int native_SDL_GetRenderDrawColor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetRenderDrawColor expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetRenderDrawColor expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        Uint8 r, g, b, a;
        SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

        vm->pushInt(r);
        vm->pushInt(g);
        vm->pushInt(b);
        vm->pushInt(a);
        return 4;
    }

    int native_SDL_RenderSetViewport(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_RenderSetViewport expects 5 arguments (renderer, x, y, w, h)");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderSetViewport expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Rect rect;
        rect.x = args[1].asNumber();
        rect.y = args[2].asNumber();
        rect.w = args[3].asNumber();
        rect.h = args[4].asNumber();

        int result = SDL_RenderSetViewport(renderer, &rect);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_RenderGetViewport(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RenderGetViewport expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderGetViewport expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Rect rect;
        SDL_RenderGetViewport(renderer, &rect);

        vm->pushInt(rect.x);
        vm->pushInt(rect.y);
        vm->pushInt(rect.w);
        vm->pushInt(rect.h);
        return 4;
       
    }

    int native_SDL_RenderSetClipRect(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_RenderSetClipRect expects 5 arguments (renderer, x, y, w, h)");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderSetClipRect expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Rect rect;
        rect.x = args[1].asNumber();
        rect.y = args[2].asNumber();
        rect.w = args[3].asNumber();
        rect.h = args[4].asNumber();

        int result = SDL_RenderSetClipRect(renderer, &rect);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_RenderGetClipRect(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RenderGetClipRect expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderGetClipRect expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Rect rect;
        SDL_RenderGetClipRect(renderer, &rect);

        vm->pushInt(rect.x);
        vm->pushInt(rect.y);
        vm->pushInt(rect.w);
        vm->pushInt(rect.h);
        return 4;
    }

    int native_SDL_RenderIsClipEnabled(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RenderIsClipEnabled expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderIsClipEnabled expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_bool result = SDL_RenderIsClipEnabled(renderer);

        vm->push(vm->makeBool(result));
        return 1;
    }

    int native_SDL_RenderSetLogicalSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("SDL_RenderSetLogicalSize expects 3 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderSetLogicalSize expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        int w = args[1].asNumber();
        int h = args[2].asNumber();

        int result = SDL_RenderSetLogicalSize(renderer, w, h);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_RenderGetLogicalSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_RenderGetLogicalSize expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_RenderGetLogicalSize expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        int w, h;
        SDL_RenderGetLogicalSize(renderer, &w, &h);

        vm->push(vm->makeInt(w));
        vm->push(vm->makeInt(h));
        return 2;
    }

    int native_SDL_GetRendererOutputSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetRendererOutputSize expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetRendererOutputSize expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        int w, h;
        SDL_GetRendererOutputSize(renderer, &w, &h);

        vm->push(vm->makeInt(w));
        vm->push(vm->makeInt(h));
        return 2;
    }

    // =====================================================
    // TEXTURE FUNCTIONS
    // =====================================================

    int native_SDL_CreateTexture(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_CreateTexture expects 5 arguments (renderer, format, access, w, h)");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_CreateTexture expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        Uint32 format = args[1].asNumber();
        int access = args[2].asNumber();
        int w = args[3].asNumber();
        int h = args[4].asNumber();

        SDL_Texture *texture = SDL_CreateTexture(renderer, format, access, w, h);
        if (!texture)
        {
            Error("SDL_CreateTexture failed");
            return 0;
        }
        vm->push(vm->makePointer(texture));
        return 1;
    }

    int native_SDL_DestroyTexture(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_DestroyTexture expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_DestroyTexture expects texture pointer");
            return 0;
        }

        SDL_Texture *texture = (SDL_Texture *)args[0].asPointer();
        SDL_DestroyTexture(texture);
        return 0;
    }

    int native_SDL_SetRenderTarget(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetRenderTarget expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetRenderTarget expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Texture *texture = nullptr;
        if (args[1].isPointer())
        {
            texture = (SDL_Texture *)args[1].asPointer();
        }

        int result = SDL_SetRenderTarget(renderer, texture);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_GetRenderTarget(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetRenderTarget expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetRenderTarget expects renderer pointer");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Texture *texture = SDL_GetRenderTarget(renderer);
       if (!texture)
        {
            
            return 1;
        }
        vm->push(vm->makePointer(texture));
        return 1;
    }

    int native_SDL_QueryTexture(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_QueryTexture expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_QueryTexture expects texture pointer");
            return 0;
        }

        SDL_Texture *texture = (SDL_Texture *)args[0].asPointer();
        Uint32 format;
        int access, w, h;
        SDL_QueryTexture(texture, &format, &access, &w, &h);

        vm->push(vm->makeInt(format));
        vm->push(vm->makeInt(access));
        vm->push(vm->makeInt(w));
        vm->push(vm->makeInt(h));
        return 4;
    }

    int native_SDL_SetTextureColorMod(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("SDL_SetTextureColorMod expects 4 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetTextureColorMod expects texture pointer");
            return 0;
        }

        SDL_Texture *texture = (SDL_Texture *)args[0].asPointer();
        Uint8 r = args[1].asNumber();
        Uint8 g = args[2].asNumber();
        Uint8 b = args[3].asNumber();

        int result = SDL_SetTextureColorMod(texture, r, g, b);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_SetTextureAlphaMod(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetTextureAlphaMod expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetTextureAlphaMod expects texture pointer");
            return 0;
        }

        SDL_Texture *texture = (SDL_Texture *)args[0].asPointer();
        Uint8 alpha = args[1].asNumber();

        int result = SDL_SetTextureAlphaMod(texture, alpha);
        vm->push(vm->makeInt(result));
        return 1;
    }

    int native_SDL_SetTextureBlendMode(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_SetTextureBlendMode expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetTextureBlendMode expects texture pointer");
            return 0;
        }

        SDL_Texture *texture = (SDL_Texture *)args[0].asPointer();
        SDL_BlendMode blendMode = (SDL_BlendMode)args[1].asNumber();

        int result = SDL_SetTextureBlendMode(texture, blendMode);
        vm->push(vm->makeInt(result));
        return 1;
    }

    // SDL_RenderCopy(renderer, texture, srcRect, dstRect)
    // srcRect pode ser nil para usar toda a textura
    // dstRect pode ser nil para usar todo o renderer
    int native_SDL_RenderCopy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 10)
        {
            Error("SDL_RenderCopy expects 10 arguments (renderer, texture, srcX, srcY, srcW, srcH, dstX, dstY, dstW, dstH)");
            return 0;
        }
        if (!args[0].isPointer() || !args[1].isPointer())
        {
            Error("SDL_RenderCopy expects renderer and texture pointers");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Texture *texture = (SDL_Texture *)args[1].asPointer();

        SDL_Rect srcRect, dstRect;
        SDL_Rect *pSrc = nullptr;
        SDL_Rect *pDst = nullptr;

        // Source rect (se srcW > 0)
        int srcW = args[4].asNumber();
        if (srcW > 0)
        {
            srcRect.x = args[2].asNumber();
            srcRect.y = args[3].asNumber();
            srcRect.w = srcW;
            srcRect.h = args[5].asNumber();
            pSrc = &srcRect;
        }

        // Dest rect (se dstW > 0)
        int dstW = args[8].asNumber();
        if (dstW > 0)
        {
            dstRect.x = args[6].asNumber();
            dstRect.y = args[7].asNumber();
            dstRect.w = dstW;
            dstRect.h = args[9].asNumber();
            pDst = &dstRect;
        }

        int result = SDL_RenderCopy(renderer, texture, pSrc, pDst);
        vm->push(vm->makeInt(result));
        return 1;
    }

    // Versão simplificada para desenhar textura inteira numa posição
    int native_SDL_RenderCopySimple(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("SDL_RenderCopySimple expects 4 arguments (renderer, texture, x, y)");
            return 0;
        }
        if (!args[0].isPointer() || !args[1].isPointer())
        {
            Error("SDL_RenderCopySimple expects renderer and texture pointers");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Texture *texture = (SDL_Texture *)args[1].asPointer();
        int x = args[2].asNumber();
        int y = args[3].asNumber();

        // Query texture size
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

        SDL_Rect dstRect = {x, y, w, h};
        int result = SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
        vm->push(vm->makeInt(result));
        return 1;
    }

    // SDL_RenderCopyEx com rotação e flip
    int native_SDL_RenderCopyEx(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 14)
        {
            Error("SDL_RenderCopyEx expects 14 arguments (renderer, texture, srcX, srcY, srcW, srcH, dstX, dstY, dstW, dstH, angle, centerX, centerY, flip)");
            return 0;
        }
        if (!args[0].isPointer() || !args[1].isPointer())
        {
            Error("SDL_RenderCopyEx expects renderer and texture pointers");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Texture *texture = (SDL_Texture *)args[1].asPointer();

        SDL_Rect srcRect, dstRect;
        SDL_Rect *pSrc = nullptr;
        SDL_Rect *pDst = nullptr;

        // Source rect (se srcW > 0)
        int srcW = args[4].asNumber();
        if (srcW > 0)
        {
            srcRect.x = args[2].asNumber();
            srcRect.y = args[3].asNumber();
            srcRect.w = srcW;
            srcRect.h = args[5].asNumber();
            pSrc = &srcRect;
        }

        // Dest rect (se dstW > 0)
        int dstW = args[8].asNumber();
        if (dstW > 0)
        {
            dstRect.x = args[6].asNumber();
            dstRect.y = args[7].asNumber();
            dstRect.w = dstW;
            dstRect.h = args[9].asNumber();
            pDst = &dstRect;
        }

        double angle = args[10].asNumber();

        SDL_Point center;
        SDL_Point *pCenter = nullptr;
        int centerX = args[11].asNumber();
        int centerY = args[12].asNumber();
        if (centerX >= 0 && centerY >= 0)
        {
            center.x = centerX;
            center.y = centerY;
            pCenter = &center;
        }

        SDL_RendererFlip flip = (SDL_RendererFlip)args[13].asNumber();

        int result = SDL_RenderCopyEx(renderer, texture, pSrc, pDst, angle, pCenter, flip);
        vm->push(vm->makeInt(result));
        return 1;
    }

    // Versão simplificada com rotação
    int native_SDL_RenderCopyExSimple(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("SDL_RenderCopyExSimple expects 6 arguments (renderer, texture, x, y, angle, flip)");
            return 0;
        }
        if (!args[0].isPointer() || !args[1].isPointer())
        {
            Error("SDL_RenderCopyExSimple expects renderer and texture pointers");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Texture *texture = (SDL_Texture *)args[1].asPointer();
        int x = args[2].asNumber();
        int y = args[3].asNumber();
        double angle = args[4].asNumber();
        SDL_RendererFlip flip = (SDL_RendererFlip)args[5].asNumber();

        // Query texture size
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

        SDL_Rect dstRect = {x, y, w, h};
        int result = SDL_RenderCopyEx(renderer, texture, nullptr, &dstRect, angle, nullptr, flip);
        vm->push(vm->makeInt(result));
        return 1;
    }

    // =====================================================
    // SURFACE FUNCTIONS (para carregar imagens)
    // =====================================================

    int native_SDL_LoadBMP(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_LoadBMP expects 1 argument");
            return 0;
        }
        if (!args[0].isString())
        {
            Error("SDL_LoadBMP expects string filename");
            return 0;
        }

        const char *filename = args[0].asStringChars();
        SDL_Surface *surface = SDL_LoadBMP(filename);
        if (surface)
        {
            vm->push(vm->makePointer((void *)surface));
        }else 
        {
            vm->push(vm->makeInt(-1));
        }
        return 1;
        
    }

    int native_SDL_FreeSurface(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_FreeSurface expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_FreeSurface expects surface pointer");
            return 0;
        }

        SDL_Surface *surface = (SDL_Surface *)args[0].asPointer();
        SDL_FreeSurface(surface);
        return 0;
    }

    int native_SDL_CreateTextureFromSurface(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("SDL_CreateTextureFromSurface expects 2 arguments");
            return 0;
        }
        if (!args[0].isPointer() || !args[1].isPointer())
        {
            Error("SDL_CreateTextureFromSurface expects renderer and surface pointers");
            return 0;
        }

        SDL_Renderer *renderer = (SDL_Renderer *)args[0].asPointer();
        SDL_Surface *surface = (SDL_Surface *)args[1].asPointer();

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture)
        {
            vm->push(vm->makePointer((void *)texture));
        }else 
        {
            vm->push(vm->makeInt(-1));
        }
        return 1;
    }

    int native_SDL_GetSurfaceSize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SDL_GetSurfaceSize expects 1 argument");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_GetSurfaceSize expects surface pointer");
            return 0;
        }

        SDL_Surface *surface = (SDL_Surface *)args[0].asPointer();

        int width = surface->w;
        int height = surface->h;
        vm->push(vm->makeInt(width));
        vm->push(vm->makeInt(height));
        return 2;
    }

    int native_SDL_SetColorKey(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("SDL_SetColorKey expects 5 arguments (surface, flag, r, g, b)");
            return 0;
        }
        if (!args[0].isPointer())
        {
            Error("SDL_SetColorKey expects surface pointer");
            return 0;
        }

        SDL_Surface *surface = (SDL_Surface *)args[0].asPointer();
        int flag = args[1].asNumber();
        Uint8 r = args[2].asNumber();
        Uint8 g = args[3].asNumber();
        Uint8 b = args[4].asNumber();

        Uint32 key = SDL_MapRGB(surface->format, r, g, b);
        int result = SDL_SetColorKey(surface, flag, key);
        vm->push(vm->makeInt(result));
        return 1;
    }

    void register_renderer(ModuleBuilder &module)
    {
        // =============================================================
        //  RENDERER FLAGS
        // =============================================================
        module.addInt("SDL_RENDERER_SOFTWARE", SDL_RENDERER_SOFTWARE)
            .addInt("SDL_RENDERER_ACCELERATED", SDL_RENDERER_ACCELERATED)
            .addInt("SDL_RENDERER_PRESENTVSYNC", SDL_RENDERER_PRESENTVSYNC)
            .addInt("SDL_RENDERER_TARGETTEXTURE", SDL_RENDERER_TARGETTEXTURE)

            // =============================================================
            //  BLEND MODES
            // =============================================================
            .addInt("SDL_BLENDMODE_NONE", SDL_BLENDMODE_NONE)
            .addInt("SDL_BLENDMODE_BLEND", SDL_BLENDMODE_BLEND)
            .addInt("SDL_BLENDMODE_ADD", SDL_BLENDMODE_ADD)
            .addInt("SDL_BLENDMODE_MOD", SDL_BLENDMODE_MOD)

            // =============================================================
            //  RENDERER FUNCTIONS
            // =============================================================
            .addFunction("SDL_CreateRenderer", native_SDL_CreateRenderer, 3)
            .addFunction("SDL_DestroyRenderer", native_SDL_DestroyRenderer, 1)
            .addFunction("SDL_SetRenderDrawColor", native_SDL_SetRenderDrawColor, 5)
            .addFunction("SDL_GetRenderDrawColor", native_SDL_GetRenderDrawColor, 1)
            .addFunction("SDL_RenderClear", native_SDL_RenderClear, 1)
            .addFunction("SDL_RenderPresent", native_SDL_RenderPresent, 1)
            .addFunction("SDL_RenderDrawPoint", native_SDL_RenderDrawPoint, 3)
            .addFunction("SDL_RenderDrawLine", native_SDL_RenderDrawLine, 5)
            .addFunction("SDL_RenderDrawRect", native_SDL_RenderDrawRect, 5)
            .addFunction("SDL_RenderFillRect", native_SDL_RenderFillRect, 5)
            .addFunction("SDL_SetRenderDrawBlendMode", native_SDL_SetRenderDrawBlendMode, 2)
            .addFunction("SDL_RenderSetScale", native_SDL_RenderSetScale, 3)
            .addFunction("SDL_RenderGetScale", native_SDL_RenderGetScale, 1)
            .addFunction("SDL_RenderSetViewport", native_SDL_RenderSetViewport, 5)
            .addFunction("SDL_RenderGetViewport", native_SDL_RenderGetViewport, 1)
            .addFunction("SDL_RenderSetClipRect", native_SDL_RenderSetClipRect, 5)
            .addFunction("SDL_RenderGetClipRect", native_SDL_RenderGetClipRect, 1)
            .addFunction("SDL_RenderIsClipEnabled", native_SDL_RenderIsClipEnabled, 1)
            .addFunction("SDL_RenderSetLogicalSize", native_SDL_RenderSetLogicalSize, 3)
            .addFunction("SDL_RenderGetLogicalSize", native_SDL_RenderGetLogicalSize, 1)
            .addFunction("SDL_GetRendererOutputSize", native_SDL_GetRendererOutputSize, 1)

            // =============================================================
            //  TEXTURE FUNCTIONS
            // =============================================================
            .addFunction("SDL_CreateTexture", native_SDL_CreateTexture, 5)
            .addFunction("SDL_DestroyTexture", native_SDL_DestroyTexture, 1)
            .addFunction("SDL_SetRenderTarget", native_SDL_SetRenderTarget, 2)
            .addFunction("SDL_GetRenderTarget", native_SDL_GetRenderTarget, 1)
            .addFunction("SDL_QueryTexture", native_SDL_QueryTexture, 1)
            .addFunction("SDL_SetTextureColorMod", native_SDL_SetTextureColorMod, 4)
            .addFunction("SDL_SetTextureAlphaMod", native_SDL_SetTextureAlphaMod, 2)
            .addFunction("SDL_SetTextureBlendMode", native_SDL_SetTextureBlendMode, 2)
            .addFunction("SDL_RenderCopy", native_SDL_RenderCopy, 10)
            .addFunction("SDL_RenderCopySimple", native_SDL_RenderCopySimple, 4)
            .addFunction("SDL_RenderCopyEx", native_SDL_RenderCopyEx, 14)
            .addFunction("SDL_RenderCopyExSimple", native_SDL_RenderCopyExSimple, 6)

            // =============================================================
            //  SURFACE FUNCTIONS
            // =============================================================
            .addFunction("SDL_LoadBMP", native_SDL_LoadBMP, 1)
            .addFunction("SDL_FreeSurface", native_SDL_FreeSurface, 1)
            .addFunction("SDL_CreateTextureFromSurface", native_SDL_CreateTextureFromSurface, 2)
            .addFunction("SDL_GetSurfaceSize", native_SDL_GetSurfaceSize, 1)
            .addFunction("SDL_SetColorKey", native_SDL_SetColorKey, 5)

            // =============================================================
            //  TEXTURE ACCESS CONSTANTS
            // =============================================================
            .addInt("SDL_TEXTUREACCESS_STATIC", SDL_TEXTUREACCESS_STATIC)
            .addInt("SDL_TEXTUREACCESS_STREAMING", SDL_TEXTUREACCESS_STREAMING)
            .addInt("SDL_TEXTUREACCESS_TARGET", SDL_TEXTUREACCESS_TARGET)

            // =============================================================
            //  PIXEL FORMAT CONSTANTS (common ones)
            // =============================================================
            .addInt("SDL_PIXELFORMAT_UNKNOWN", SDL_PIXELFORMAT_UNKNOWN)
            .addInt("SDL_PIXELFORMAT_RGBA8888", SDL_PIXELFORMAT_RGBA8888)
            .addInt("SDL_PIXELFORMAT_ARGB8888", SDL_PIXELFORMAT_ARGB8888)
            .addInt("SDL_PIXELFORMAT_BGRA8888", SDL_PIXELFORMAT_BGRA8888)
            .addInt("SDL_PIXELFORMAT_RGB888", SDL_PIXELFORMAT_RGB888)
            .addInt("SDL_PIXELFORMAT_RGB24", SDL_PIXELFORMAT_RGB24)
            .addInt("SDL_PIXELFORMAT_BGR24", SDL_PIXELFORMAT_BGR24)

            // =============================================================
            //  FLIP CONSTANTS
            // =============================================================
            .addInt("SDL_FLIP_NONE", SDL_FLIP_NONE)
            .addInt("SDL_FLIP_HORIZONTAL", SDL_FLIP_HORIZONTAL)
            .addInt("SDL_FLIP_VERTICAL", SDL_FLIP_VERTICAL);
    }
}