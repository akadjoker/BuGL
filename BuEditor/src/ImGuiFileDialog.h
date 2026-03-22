#pragma once

#include "imgui.h"

#include <filesystem>
#include <string>

class ImGuiFileDialog
{
public:
    enum class Mode
    {
        OpenFile,
        SaveFile,
        ChooseFolder
    };

    struct Result
    {
        bool accepted = false;
        Mode mode = Mode::OpenFile;
        std::filesystem::path path;
    };

    struct Entry
    {
        std::string label;
        std::filesystem::path path;
        bool is_directory = false;
        uintmax_t file_size = 0;
        std::filesystem::file_time_type modified_at = std::filesystem::file_time_type::min();
    };

    enum class ViewMode
    {
        List,
        Details,
        Grid
    };

    void Open(Mode mode,
              const std::filesystem::path& start_directory,
              const std::string& initial_name = std::string());

    bool Render(const std::filesystem::path& project_directory,
                const std::filesystem::path& scripts_directory,
                const std::filesystem::path& bin_directory);

    bool HasResult() const;
    Result ConsumeResult();

private:
    static std::filesystem::path SanitizeDirectory(const std::filesystem::path& path);
    static std::string FileNameFromPath(const std::string& path);
    static std::string PopupTitle(Mode mode);
    static std::string ConfirmLabel(Mode mode);

    void AcceptCurrentSelection();
    void Cancel();
    void SelectEntry(const Entry& entry);
    void OpenEntry(const Entry& entry);
    bool IsSelectedEntry(const Entry& entry) const;

    bool mRequestedOpen = false;
    bool mFocusFilename = false;
    bool mHasResult = false;
    Mode mMode = Mode::OpenFile;
    std::filesystem::path mCurrentDirectory;
    std::filesystem::path mSelectedPath;
    std::string mFileName;
    Result mResult;
    ViewMode mViewMode = ViewMode::Details;
};
