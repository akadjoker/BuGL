#include "pch.h"
#include "device.hpp"
#include "input.hpp"
#include "utils.hpp"
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
// ============================================================================
// DEVICE
// ============================================================================

Device::Device()
    : m_window(nullptr), m_context(nullptr), m_width(0), m_height(0), m_running(false), m_ready(false), m_lastTick(0), m_deltaTime(0.0f), m_fps(0), m_frameCount(0), m_fpsTimer(0), m_initialized(false), m_resize(false)
{
}

Device::~Device()
{
    Close();
}

Device &Device::Instance()
{
    static Device instance;
    return instance;
}

bool Device::Init(const char *title, int width, int height, unsigned int flags)
{
    if (m_ready)
    {
        LogWarning(" Init - Already initialized\n");
        return true;
    }

    // ---- SDL Init ----
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0)
    {
        LogError(" Init - SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    // ---- Window ----
    unsigned int windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (flags & SDL_WINDOW_RESIZABLE)
        windowFlags |= SDL_WINDOW_RESIZABLE;
    if (flags & SDL_WINDOW_FULLSCREEN)
        windowFlags |= SDL_WINDOW_FULLSCREEN;

    m_window = SDL_CreateWindow(
        title ? title : "Device",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        windowFlags);

    if (!m_window)
    {
        LogError("   SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // ---- GL Context ----
    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context)
    {
        LogError("   SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        SDL_Quit();
        return false;
    }

    SDL_GL_MakeCurrent(m_window, m_context);

    // VSync
    SDL_GL_SetSwapInterval(1);

    Input::Init();

    m_width = width;
    m_height = height;

    // ---- Log GL info ----
 
    LogInfo("  Vendor:   %s", glGetString(GL_VENDOR));
    LogInfo("  Renderer: %s", glGetString(GL_RENDERER));
    LogInfo("  Version:  %s", glGetString(GL_VERSION));
    LogInfo("  GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    m_lastTick = SDL_GetTicks();
    m_fpsTimer = m_lastTick;
    m_running = true;
    m_ready = true;
    m_resize = false;
    m_initialized = true;

    return true;
}

void Device::Close()
{
    if (!m_ready)
        return;
    
    m_initialized = false;  

    Input::Shutdown();

    if (m_context)
    {
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();

    m_running = false;
    m_ready = false;
 
}

bool Device::Running()
{
    if (!m_running)
        return false;

    // ---- Delta time ----
    unsigned int now = SDL_GetTicks();
    unsigned int elapsed = now - m_lastTick;
    m_deltaTime = (float)elapsed / 1000.0f;
    if (m_deltaTime > 0.1f)
        m_deltaTime = 0.1f; // clamp on long stalls
    m_lastTick = now;

    // ---- FPS counter ----
    m_frameCount++;
    if (now - m_fpsTimer >= 1000)
    {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fpsTimer = now;
    }

    // ---- Events ----
    Input::Update();
    ProcessEvents();

    return m_running;
}

void Device::ProcessEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            m_running = false;
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                m_width = event.window.data1;
                m_height = event.window.data2;
                m_resize = true;
            }
            break;

        case SDL_KEYDOWN:
        {
            Input::OnKeyDown(event.key);
            break;
        }
        case SDL_KEYUP:
        {
            Input::OnKeyUp(event.key);
            break;
        }
        case SDL_TEXTINPUT:
        {
            Input::OnTextInput(event.text);
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {

            Input::OnMouseDown(event.button);
        }
        break;
        case SDL_MOUSEBUTTONUP:
        {
            Input::OnMouseUp(event.button);

            break;
        }
        case SDL_MOUSEMOTION:
        {
            Input::OnMouseMove(event.motion);
            break;
        }

        case SDL_MOUSEWHEEL:
        {
            Input::OnMouseWheel(event.wheel);
            break;
        }
        case SDL_CONTROLLERDEVICEADDED:
        {
            Input::OnGamepadDeviceAdded(event.cdevice);
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED:
        {
            Input::OnGamepadDeviceRemoved(event.cdevice);
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:
        {
            Input::OnGamepadButtonDown(event.cbutton);
            break;
        }
        case SDL_CONTROLLERBUTTONUP:
        {
            Input::OnGamepadButtonUp(event.cbutton);
            break;
        }
        case SDL_CONTROLLERAXISMOTION:
        {
            Input::OnGamepadAxisMotion(event.caxis);
            break;
        }
        default:
            break;
        }
    }
}

void Device::Flip()
{
    SDL_GL_SwapWindow(m_window);
    m_resize = false;
}

void Device::Quit()
{
    m_running = false;
}

void Device::SetTitle(const char *title)
{
    if (m_window && title)
        SDL_SetWindowTitle(m_window, title);
}

void Device::SetSize(int width, int height)
{
    if (m_window)
    {
        SDL_SetWindowSize(m_window, width, height);
        m_width = width;
        m_height = height;
    }
}
