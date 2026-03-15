#include "pch.h"
#include "device.hpp"
#include "input.hpp"
#include "utils.hpp"
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <ctime>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace
{
    static bool create_directory_single(const char *path)
    {
        if (!path || !path[0])
            return true;

#ifdef _WIN32
        if (_mkdir(path) == 0 || errno == EEXIST)
            return true;
#else
        if (mkdir(path, 0755) == 0 || errno == EEXIST)
            return true;
#endif
        return false;
    }

    static bool ensure_parent_directories(const char *path)
    {
        if (!path || !path[0])
            return false;

        char buffer[512];
        std::snprintf(buffer, sizeof(buffer), "%s", path);

        char *lastSlash = std::strrchr(buffer, '/');
#ifdef _WIN32
        char *lastBackslash = std::strrchr(buffer, '\\');
        if (!lastSlash || (lastBackslash && lastBackslash > lastSlash))
            lastSlash = lastBackslash;
#endif
        if (!lastSlash)
            return true;

        *lastSlash = '\0';
        if (!buffer[0])
            return true;

        for (char *cursor = buffer + 1; *cursor; ++cursor)
        {
            if (*cursor == '/' || *cursor == '\\')
            {
                char saved = *cursor;
                *cursor = '\0';
                if (buffer[0] && !create_directory_single(buffer))
                    return false;
                *cursor = saved;
            }
        }

        return create_directory_single(buffer);
    }
}
// ============================================================================
// DEVICE
// ============================================================================

Device::Device()
    : m_window(nullptr), m_context(nullptr), m_width(0), m_height(0), m_running(false), m_ready(false), m_initialized(false), m_resize(false), m_imguiInitialized(false),
      m_gifRecording(false), m_gifWidth(0), m_gifHeight(0), m_gifFrameCentis(5), m_gifMaxBitDepth(16), m_gifSampleEvery(2), m_gifSampleCounter(0),
      m_gifFile(nullptr), m_gifState({}), m_gifPixels(nullptr), m_gifPixelBytes(0), m_lastTick(0), m_deltaTime(0.0f), m_fps(0), m_frameCount(0), m_fpsTimer(0)
{
    m_gifPath[0] = '\0';
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
    StopGifCapture();
    ShutdownImGui();

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

bool Device::InitImGui()
{
    if (!m_ready)
    {
        LogError(" InitImGui - Device is not initialized");
        return false;
    }

    if (m_imguiInitialized)
        return true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForOpenGL(m_window, m_context))
    {
        LogError(" InitImGui - ImGui SDL backend init failed");
        ImGui::DestroyContext();
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330"))
    {
        LogError(" InitImGui - ImGui OpenGL backend init failed");
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_imguiInitialized = true;
    return true;
}

void Device::ShutdownImGui()
{
    if (!m_imguiInitialized)
        return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    m_imguiInitialized = false;
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

    if (m_imguiInitialized)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    return m_running;
}

void Device::ProcessEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (m_imguiInitialized)
            ImGui_ImplSDL2_ProcessEvent(&event);

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
            if (!event.key.repeat && event.key.keysym.sym == SDLK_F10)
                ToggleGifCapture();
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
    if (m_imguiInitialized)
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    if (m_gifRecording)
        CaptureGifFrame();

    SDL_GL_SwapWindow(m_window);
    m_resize = false;
}

ImGuiContext *Device::GetImGuiContext() const
{
    return m_imguiInitialized ? ImGui::GetCurrentContext() : nullptr;
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

void Device::ResetGifCaptureState()
{
    if (m_gifFile)
    {
        std::fclose(m_gifFile);
        m_gifFile = nullptr;
    }

    m_gifState = {};
    if (m_gifPixels)
    {
        std::free(m_gifPixels);
        m_gifPixels = nullptr;
    }
    m_gifPixelBytes = 0;
    m_gifPath[0] = '\0';
    m_gifRecording = false;
    m_gifWidth = 0;
    m_gifHeight = 0;
    m_gifSampleCounter = 0;
}

void Device::MakeGifCapturePath(char *buffer, size_t bufferSize) const
{
    if (!buffer || bufferSize == 0)
        return;

    std::time_t now = std::time(nullptr);
    std::tm tmValue = {};
#if defined(_WIN32)
    localtime_s(&tmValue, &now);
#else
    localtime_r(&now, &tmValue);
#endif

    char nameBuffer[64];
    strftime(nameBuffer, sizeof(nameBuffer), "%Y-%m-%d_%H-%M-%S.gif", &tmValue);
    std::snprintf(buffer, bufferSize, "gif/%s", nameBuffer);
}

bool Device::StartGifCapture(const char *path)
{
    if (!m_ready)
    {
        LogWarning(" GIF capture ignored: device not ready");
        return false;
    }

    if (m_gifRecording)
        return true;

    const int captureWidth = (m_width > 0) ? m_width : 1;
    const int captureHeight = (m_height > 0) ? m_height : 1;
    const size_t totalBytes = (size_t)captureWidth * (size_t)captureHeight * 4u;
    char outputPath[512];
    if (path && path[0] != '\0')
        std::snprintf(outputPath, sizeof(outputPath), "%s", path);
    else
        MakeGifCapturePath(outputPath, sizeof(outputPath));

    if (!ensure_parent_directories(outputPath))
    {
        LogError(" GIF capture failed to create directory for: %s", outputPath);
        return false;
    }

    FILE *file = std::fopen(outputPath, "wb");
    if (!file)
    {
        LogError(" GIF capture failed to open file: %s", outputPath);
        return false;
    }

    m_gifPixels = (unsigned char *)std::malloc(totalBytes);
    if (!m_gifPixels)
    {
        LogError(" GIF capture failed to allocate %zu bytes", totalBytes);
        std::fclose(file);
        return false;
    }
    std::memset(m_gifPixels, 0, totalBytes);
    m_gifPixelBytes = totalBytes;
    m_gifState = {};
    if (!msf_gif_begin_to_file(&m_gifState, captureWidth, captureHeight, (MsfGifFileWriteFunc)std::fwrite, (void *)file))
    {
        LogError(" GIF capture failed to begin encoder");
        std::fclose(file);
        std::free(m_gifPixels);
        m_gifPixels = nullptr;
        m_gifPixelBytes = 0;
        m_gifState = {};
        return false;
    }

    m_gifFile = file;
    std::snprintf(m_gifPath, sizeof(m_gifPath), "%s", outputPath);
    m_gifRecording = true;
    m_gifWidth = captureWidth;
    m_gifHeight = captureHeight;
    m_gifSampleCounter = 0;
    LogInfo(" GIF capture started: %s", m_gifPath);
    return true;
}

bool Device::StopGifCapture()
{
    if (!m_gifRecording)
        return false;

    const bool ok = msf_gif_end_to_file(&m_gifState) != 0;
    if (!ok)
        LogError(" GIF capture failed to finalize: %s", m_gifPath);

    char savedPath[512];
    std::snprintf(savedPath, sizeof(savedPath), "%s", m_gifPath);
    ResetGifCaptureState();
    if (ok)
        LogInfo(" GIF capture saved: %s", savedPath);
    return ok;
}

bool Device::ToggleGifCapture(const char *path)
{
    if (m_gifRecording)
        return StopGifCapture();
    return StartGifCapture(path);
}

bool Device::CaptureGifFrame()
{
    if (!m_gifRecording)
        return false;

    if (m_width != m_gifWidth || m_height != m_gifHeight)
    {
        LogWarning(" GIF capture stopped after resize: %dx%d -> %dx%d", m_gifWidth, m_gifHeight, m_width, m_height);
        StopGifCapture();
        return false;
    }

    m_gifSampleCounter++;
    if (m_gifSampleCounter < m_gifSampleEvery)
        return true;

    m_gifSampleCounter = 0;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_gifWidth, m_gifHeight, GL_RGBA, GL_UNSIGNED_BYTE, m_gifPixels);

    if (!msf_gif_frame_to_file(&m_gifState, m_gifPixels, m_gifFrameCentis, m_gifMaxBitDepth, -m_gifWidth * 4))
    {
        LogError(" GIF capture failed to write frame: %s", m_gifPath);
        StopGifCapture();
        return false;
    }

    return true;
}
