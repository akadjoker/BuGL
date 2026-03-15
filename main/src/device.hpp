#pragma once
#include "config.hpp"
#include <SDL2/SDL.h> 
#include "msf_gif.h"
#include <cstdio>

struct ImGuiContext;

// ============================================================================
// DEVICE (Singleton)
// ============================================================================
 

class Device
{
private:
    SDL_Window*   m_window;
    SDL_GLContext  m_context;
    int m_width;
    int m_height;
    bool m_running;
    bool m_ready;
    bool m_initialized;
    bool m_resize;
    bool m_imguiInitialized;
    bool m_gifRecording;
    int m_gifWidth;
    int m_gifHeight;
    int m_gifFrameCentis;
    int m_gifMaxBitDepth;
    int m_gifSampleEvery;
    int m_gifSampleCounter;
    FILE *m_gifFile;
    MsfGifState m_gifState;
    unsigned char *m_gifPixels;
    size_t m_gifPixelBytes;
    char m_gifPath[512];

 
    // Timing
    unsigned int m_lastTick;
    float m_deltaTime;
    unsigned int m_fps;
    unsigned int m_frameCount;
    unsigned int m_fpsTimer;

    Device();
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    void ProcessEvents();
    void ResetGifCaptureState();
    bool CaptureGifFrame();
    void MakeGifCapturePath(char *buffer, size_t bufferSize) const;

public:
    static Device& Instance();

    /// Initialize SDL2, create window and GL ES 3.0 context
    bool Init(const char* title, int width, int height, unsigned int flags = 0);

    /// Shutdown everything
    void Close();

    bool InitImGui();
    void ShutdownImGui();

    /// Poll events, update delta time. Returns false if should quit.
    bool Running();

     
    void Flip();
    bool StartGifCapture(const char *path = nullptr);
    bool StopGifCapture();
    bool ToggleGifCapture(const char *path = nullptr);

    
    /// Run the main loop using the current stage. Returns when done.
    void Run();

    /// Request exit
    void Quit();

    bool IsResized() const { return m_resize; }

    // Getters
    SDL_Window*  GetWindow()  const { return m_window; }
    SDL_GLContext GetContext() const { return m_context; }
    int GetWidth()  const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetDeltaTime() const { return m_deltaTime; }
    unsigned int GetFPS() const { return m_fps; }
    bool IsReady() const { return m_ready; }
    bool HasImGui() const { return m_imguiInitialized; }
    bool IsGifRecording() const { return m_gifRecording; }
    ImGuiContext *GetImGuiContext() const;

 
    void SetTitle(const char* title);

 
    void SetSize(int width, int height);
};
