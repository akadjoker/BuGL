#include "GifRecorder.h"

#include <SDL2/SDL_opengl.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

#define MSF_GIF_IMPL
#include "msf_gif.h"

namespace
{
std::filesystem::path MakeGifCapturePath(const std::filesystem::path& project_dir)
{
    std::time_t now = std::time(nullptr);
    std::tm tm_value = {};
#if defined(_WIN32)
    localtime_s(&tm_value, &now);
#else
    localtime_r(&now, &tm_value);
#endif

    char name_buffer[64];
    std::strftime(name_buffer, sizeof(name_buffer), "bueditor_%Y-%m-%d_%H-%M-%S.gif", &tm_value);
    return project_dir / "gif" / name_buffer;
}
}

struct GifRecorder::Impl
{
    bool recording = false;
    int width = 0;
    int height = 0;
    int frame_centis = 5;
    int max_bit_depth = 16;
    int sample_every = 2;
    int sample_counter = 0;
    FILE* file = nullptr;
    MsfGifState state = {};
    std::vector<unsigned char> pixels;
    std::filesystem::path path;

    void Reset()
    {
        if (file != nullptr)
        {
            std::fclose(file);
            file = nullptr;
        }

        state = {};
        pixels.clear();
        recording = false;
        width = 0;
        height = 0;
        sample_counter = 0;
        path.clear();
    }
};

GifRecorder::GifRecorder()
    : m_impl(std::make_unique<Impl>())
{
}

GifRecorder::~GifRecorder()
{
    std::string ignored_status;
    Stop(ignored_status);
}

bool GifRecorder::Start(const std::filesystem::path& project_dir, int width, int height, std::string& status)
{
    if (m_impl->recording)
    {
        return true;
    }
    if (width <= 0 || height <= 0)
    {
        status = "GIF capture ignored: invalid framebuffer size";
        return false;
    }

    const std::filesystem::path output_path = MakeGifCapturePath(project_dir);
    std::error_code ec;
    std::filesystem::create_directories(output_path.parent_path(), ec);
    if (ec)
    {
        status = "GIF capture failed to create directory: " + output_path.parent_path().string();
        return false;
    }

    FILE* file = std::fopen(output_path.string().c_str(), "wb");
    if (file == nullptr)
    {
        status = "GIF capture failed to open: " + output_path.string();
        return false;
    }

    m_impl->pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, 0);
    m_impl->state = {};
    if (!msf_gif_begin_to_file(&m_impl->state, width, height, (MsfGifFileWriteFunc)std::fwrite, (void*)file))
    {
        std::fclose(file);
        m_impl->pixels.clear();
        status = "GIF capture failed to start encoder";
        return false;
    }

    m_impl->file = file;
    m_impl->recording = true;
    m_impl->width = width;
    m_impl->height = height;
    m_impl->sample_counter = 0;
    m_impl->path = output_path;
    status = "GIF capture started: " + output_path.string();
    return true;
}

bool GifRecorder::Stop(std::string& status)
{
    if (!m_impl->recording)
    {
        return false;
    }

    const std::string saved_path = m_impl->path.string();
    const bool ok = msf_gif_end_to_file(&m_impl->state) != 0;
    m_impl->Reset();
    status = ok
        ? "GIF capture saved: " + saved_path
        : "GIF capture failed to finalize: " + saved_path;
    return ok;
}

bool GifRecorder::Toggle(const std::filesystem::path& project_dir, int width, int height, std::string& status)
{
    if (m_impl->recording)
    {
        return Stop(status);
    }
    return Start(project_dir, width, height, status);
}

bool GifRecorder::CaptureFrame(int width, int height, std::string& status)
{
    if (!m_impl->recording)
    {
        return false;
    }
    if (width != m_impl->width || height != m_impl->height)
    {
        const std::string saved_path = m_impl->path.string();
        Stop(status);
        status = "GIF capture stopped after resize: " + saved_path;
        return false;
    }

    m_impl->sample_counter++;
    if (m_impl->sample_counter < m_impl->sample_every)
    {
        return true;
    }
    m_impl->sample_counter = 0;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_impl->width, m_impl->height, GL_RGBA, GL_UNSIGNED_BYTE, m_impl->pixels.data());

    if (!msf_gif_frame_to_file(&m_impl->state,
                               m_impl->pixels.data(),
                               m_impl->frame_centis,
                               m_impl->max_bit_depth,
                               -m_impl->width * 4))
    {
        const std::string saved_path = m_impl->path.string();
        Stop(status);
        status = "GIF capture failed to write frame: " + saved_path;
        return false;
    }

    return true;
}

bool GifRecorder::IsRecording() const
{
    return m_impl->recording;
}

std::string GifRecorder::GetPath() const
{
    return m_impl->path.string();
}
