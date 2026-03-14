#pragma once
#include "config.hpp"
#include <SDL2/SDL.h> 

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
    ImGuiContext *GetImGuiContext() const;

 
    void SetTitle(const char* title);

 
    void SetSize(int width, int height);
};
