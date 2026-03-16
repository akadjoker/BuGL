#pragma once

#include <filesystem>
#include <memory>
#include <string>

class GifRecorder
{
public:
    GifRecorder();
    ~GifRecorder();

    GifRecorder(const GifRecorder&) = delete;
    GifRecorder& operator=(const GifRecorder&) = delete;

    bool Start(const std::filesystem::path& project_dir, int width, int height, std::string& status);
    bool Stop(std::string& status);
    bool Toggle(const std::filesystem::path& project_dir, int width, int height, std::string& status);
    bool CaptureFrame(int width, int height, std::string& status);

    bool IsRecording() const;
    std::string GetPath() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
