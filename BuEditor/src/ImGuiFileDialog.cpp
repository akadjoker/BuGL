#include "ImGuiFileDialog.h"
#include "ImGuiFontAwesome.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <system_error>
#include <vector>

namespace
{
using FileClock = std::filesystem::file_time_type;

std::string FormatSize(uintmax_t size)
{
    static const char* suffixes[] = {"B", "KB", "MB", "GB"};
    double value = static_cast<double>(size);
    size_t suffix_index = 0;
    while (value >= 1024.0 && suffix_index + 1 < (sizeof(suffixes) / sizeof(suffixes[0])))
    {
        value /= 1024.0;
        ++suffix_index;
    }

    char buffer[32];
    if (suffix_index == 0)
    {
        std::snprintf(buffer, sizeof(buffer), "%llu %s", static_cast<unsigned long long>(size), suffixes[suffix_index]);
    }
    else
    {
        std::snprintf(buffer, sizeof(buffer), "%.1f %s", value, suffixes[suffix_index]);
    }
    return buffer;
}

std::string FormatTimestamp(const FileClock& timestamp)
{
    if (timestamp == FileClock::min())
    {
        return "-";
    }

    using namespace std::chrono;
    const auto system_time = time_point_cast<std::chrono::system_clock::duration>(
        timestamp - FileClock::clock::now() + std::chrono::system_clock::now());
    const std::time_t time_value = std::chrono::system_clock::to_time_t(system_time);

    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &time_value);
#else
    localtime_r(&time_value, &local_tm);
#endif

    char buffer[32];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &local_tm) == 0)
    {
        return "-";
    }
    return buffer;
}

std::vector<ImGuiFileDialog::Entry> ListDirectoryEntries(const std::filesystem::path& directory)
{
    std::vector<ImGuiFileDialog::Entry> entries;
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec))
    {
        return entries;
    }

    for (const auto& entry : std::filesystem::directory_iterator(
             directory,
             std::filesystem::directory_options::skip_permission_denied,
             ec))
    {
        if (ec)
        {
            break;
        }

        std::error_code type_error;
        ImGuiFileDialog::Entry dialog_entry;
        dialog_entry.path = entry.path();
        dialog_entry.label = dialog_entry.path.filename().string();
        dialog_entry.is_directory = !type_error && entry.is_directory(type_error);

        std::error_code size_error;
        if (!dialog_entry.is_directory)
        {
            dialog_entry.file_size = entry.file_size(size_error);
            if (size_error)
            {
                dialog_entry.file_size = 0;
            }
        }

        std::error_code time_error;
        dialog_entry.modified_at = entry.last_write_time(time_error);
        if (time_error)
        {
            dialog_entry.modified_at = FileClock::min();
        }

        entries.push_back(dialog_entry);
    }

    std::sort(entries.begin(), entries.end(), [](const ImGuiFileDialog::Entry& lhs, const ImGuiFileDialog::Entry& rhs)
    {
        if (lhs.is_directory != rhs.is_directory)
        {
            return lhs.is_directory > rhs.is_directory;
        }
        return lhs.label < rhs.label;
    });

    return entries;
}

bool IsSelectedEntry(const ImGuiFileDialog::Entry& entry,
                     ImGuiFileDialog::Mode mode,
                     const std::filesystem::path& selected_path,
                     const std::string& file_name)
{
    return (!entry.is_directory && file_name == entry.label) ||
           (entry.is_directory && mode == ImGuiFileDialog::Mode::ChooseFolder && entry.path == selected_path) ||
           (entry.path == selected_path);
}

std::string BuildEntryLabel(const ImGuiFileDialog::Entry& entry)
{
    const bool use_icons = ImGuiFontAwesome::IsLoaded();
    const bool is_script = entry.path.extension() == ".bu";
    const bool is_bytecode = entry.path.extension() == ".buc";

    const char* prefix = nullptr;
    if (entry.is_directory)
    {
        prefix = use_icons ? ImGuiFontAwesome::kFolder : nullptr;
    }
    else if (is_script)
    {
        prefix = use_icons ? ImGuiFontAwesome::kFileCode : nullptr;
    }
    else if (is_bytecode)
    {
        prefix = use_icons ? ImGuiFontAwesome::kFileLines : nullptr;
    }
    else
    {
        prefix = use_icons ? ImGuiFontAwesome::kFile : nullptr;
    }

    std::string label;
    if (prefix != nullptr)
    {
        label = prefix;
        label += " ";
    }
    label += entry.label;
    if (entry.is_directory && !use_icons)
    {
        label += "/";
    }
    return label;
}

ImU32 GetEntryColor(const ImGuiFileDialog::Entry& entry)
{
    if (entry.is_directory)
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(0.88f, 0.76f, 0.36f, 1.0f));
    }

    if (entry.path.extension() == ".bu")
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(0.49f, 0.82f, 0.58f, 1.0f));
    }

    if (entry.path.extension() == ".buc")
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(0.49f, 0.70f, 0.92f, 1.0f));
    }

    return ImGui::GetColorU32(ImGuiCol_Text);
}

void RenderEntryLabel(const ImGuiFileDialog::Entry& entry)
{
    const std::string label = BuildEntryLabel(entry);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(GetEntryColor(entry)));
    ImGui::TextUnformatted(label.c_str());
    ImGui::PopStyleColor();
}
}

std::filesystem::path ImGuiFileDialog::SanitizeDirectory(const std::filesystem::path& path)
{
    std::error_code ec;
    if (!path.empty() && std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec))
    {
        return path.lexically_normal();
    }

    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec)
    {
        return cwd.lexically_normal();
    }

    return path.lexically_normal();
}

std::string ImGuiFileDialog::FileNameFromPath(const std::string& path)
{
    return std::filesystem::path(path).filename().string();
}

std::string ImGuiFileDialog::PopupTitle(Mode mode)
{
    switch (mode)
    {
    case Mode::OpenFile: return "Open File";
    case Mode::SaveFile: return "Save File";
    case Mode::ChooseFolder: return "Choose Folder";
    }
    return "File Browser";
}

std::string ImGuiFileDialog::ConfirmLabel(Mode mode)
{
    switch (mode)
    {
    case Mode::OpenFile: return "Open";
    case Mode::SaveFile: return "Save";
    case Mode::ChooseFolder: return "Select";
    }
    return "Confirm";
}

void ImGuiFileDialog::Open(Mode mode,
                           const std::filesystem::path& start_directory,
                           const std::string& initial_name)
{
    mMode = mode;
    mCurrentDirectory = SanitizeDirectory(start_directory);
    mSelectedPath.clear();
    mFileName = initial_name;
    if (mMode == Mode::ChooseFolder)
    {
        mFileName.clear();
        mSelectedPath = mCurrentDirectory;
    }
    mRequestedOpen = true;
    mFocusFilename = true;
    mHasResult = false;
}

bool ImGuiFileDialog::HasResult() const
{
    return mHasResult;
}

ImGuiFileDialog::Result ImGuiFileDialog::ConsumeResult()
{
    mHasResult = false;
    return mResult;
}

void ImGuiFileDialog::AcceptCurrentSelection()
{
    mResult.accepted = true;
    mResult.mode = mMode;
    if (mMode == Mode::ChooseFolder)
    {
        mResult.path = mSelectedPath.empty() ? mCurrentDirectory : mSelectedPath;
    }
    else
    {
        mResult.path = (mCurrentDirectory / mFileName).lexically_normal();
    }
    mHasResult = true;
}

void ImGuiFileDialog::Cancel()
{
    mResult = {};
    mHasResult = false;
}

void ImGuiFileDialog::SelectEntry(const Entry& entry)
{
    mSelectedPath = entry.path;
    if (!entry.is_directory && mMode != Mode::ChooseFolder)
    {
        mFileName = entry.label;
    }
}

void ImGuiFileDialog::OpenEntry(const Entry& entry)
{
    if (entry.is_directory)
    {
        mCurrentDirectory = entry.path;
        if (mMode == Mode::ChooseFolder)
        {
            mSelectedPath = mCurrentDirectory;
        }
        else
        {
            mSelectedPath.clear();
        }
        return;
    }

    if (mMode != Mode::ChooseFolder)
    {
        mFileName = entry.label;
        if (mMode == Mode::OpenFile)
        {
            AcceptCurrentSelection();
        }
    }
}

bool ImGuiFileDialog::IsSelectedEntry(const Entry& entry) const
{
    return ::IsSelectedEntry(entry, mMode, mSelectedPath, mFileName);
}

bool ImGuiFileDialog::Render(const std::filesystem::path& project_directory,
                             const std::filesystem::path& scripts_directory,
                             const std::filesystem::path& bin_directory)
{
    const std::string popup_title = PopupTitle(mMode);
    bool changed = false;

    if (mRequestedOpen)
    {
        ImGui::OpenPopup(popup_title.c_str());
        mRequestedOpen = false;
    }

    if (!ImGui::BeginPopupModal(popup_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return false;
    }

    if (ImGui::Button("Project"))
    {
        mCurrentDirectory = SanitizeDirectory(project_directory);
    }
    ImGui::SameLine();
    if (ImGui::Button("Scripts"))
    {
        mCurrentDirectory = SanitizeDirectory(scripts_directory);
    }
    ImGui::SameLine();
    if (ImGui::Button("Bin"))
    {
        mCurrentDirectory = SanitizeDirectory(bin_directory);
    }
    ImGui::SameLine();
    if (ImGui::Button("Up"))
    {
        const std::filesystem::path parent = mCurrentDirectory.parent_path();
        if (!parent.empty())
        {
            mCurrentDirectory = parent.lexically_normal();
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("View");
    ImGui::SameLine();
    ImGui::RadioButton("List", reinterpret_cast<int*>(&mViewMode), static_cast<int>(ViewMode::List));
    ImGui::SameLine();
    ImGui::RadioButton("Details", reinterpret_cast<int*>(&mViewMode), static_cast<int>(ViewMode::Details));
    ImGui::SameLine();
    ImGui::RadioButton("Grid", reinterpret_cast<int*>(&mViewMode), static_cast<int>(ViewMode::Grid));

    ImGui::Separator();
    ImGui::TextWrapped("%s", mCurrentDirectory.string().c_str());

    const std::vector<Entry> entries = ListDirectoryEntries(mCurrentDirectory);
    ImGui::BeginChild("##file_dialog_entries", ImVec2(760.0f, 320.0f), true);

    if (mViewMode == ViewMode::Details)
    {
        if (ImGui::BeginTable("##file_dialog_table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.60f);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableHeadersRow();

            for (const Entry& entry : entries)
            {
                const bool is_selected = IsSelectedEntry(entry);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::PushID(entry.path.string().c_str());
                if (ImGui::Selectable("##entry", is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
                {
                    SelectEntry(entry);
                }
                if (ImGui::IsItemVisible())
                {
                    ImGui::SameLine(0.0f, 8.0f);
                    RenderEntryLabel(entry);
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    OpenEntry(entry);
                    if (!entry.is_directory && mMode == Mode::OpenFile)
                    {
                        ImGui::CloseCurrentPopup();
                        changed = true;
                    }
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(entry.is_directory ? "-" : FormatSize(entry.file_size).c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(FormatTimestamp(entry.modified_at).c_str());
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    else if (mViewMode == ViewMode::Grid)
    {
        const float cell_min_width = 150.0f;
        const float tile_height = 72.0f;
        const float available_width = ImGui::GetContentRegionAvail().x;
        const int columns = std::max(1, static_cast<int>(available_width / cell_min_width));
        if (ImGui::BeginTable("##file_dialog_grid", columns, ImGuiTableFlags_SizingStretchSame))
        {
            for (const Entry& entry : entries)
            {
                ImGui::TableNextColumn();
                ImGui::PushID(entry.path.string().c_str());
                const bool is_selected = IsSelectedEntry(entry);
                const ImVec2 tile_min = ImGui::GetCursorScreenPos();
                const float tile_width = ImGui::GetContentRegionAvail().x;
                if (ImGui::InvisibleButton("##grid_entry", ImVec2(tile_width, tile_height)))
                {
                    SelectEntry(entry);
                }
                const bool is_hovered = ImGui::IsItemHovered();
                if (is_hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    OpenEntry(entry);
                    if (!entry.is_directory && mMode == Mode::OpenFile)
                    {
                        ImGui::CloseCurrentPopup();
                        changed = true;
                    }
                }
                if (ImGui::IsItemVisible())
                {
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    const ImVec2 min = ImGui::GetItemRectMin();
                    const ImVec2 max = ImGui::GetItemRectMax();
                    const ImU32 label_color = GetEntryColor(entry);
                    const ImU32 meta_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
                    const std::string label = BuildEntryLabel(entry);
                    const ImU32 bg_color =
                        is_selected ? ImGui::GetColorU32(ImGuiCol_HeaderActive) :
                        is_hovered ? ImGui::GetColorU32(ImGuiCol_HeaderHovered) :
                        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

                    if ((bg_color & IM_COL32_A_MASK) != 0)
                    {
                        draw_list->AddRectFilled(tile_min,
                                                 ImVec2(tile_min.x + tile_width, tile_min.y + tile_height),
                                                 bg_color,
                                                 6.0f);
                    }

                    draw_list->AddText(ImVec2(min.x + 10.0f, min.y + 10.0f),
                                       label_color,
                                       label.c_str());

                    const std::string meta_text = entry.is_directory ? "Folder" : FormatSize(entry.file_size);
                    draw_list->AddText(ImVec2(min.x + 10.0f, max.y - ImGui::GetTextLineHeight() - 8.0f),
                                       meta_color,
                                       meta_text.c_str());
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    else
    {
        for (const Entry& entry : entries)
        {
            const bool is_selected = IsSelectedEntry(entry);
            ImGui::PushID(entry.path.string().c_str());
            if (ImGui::Selectable("##entry", is_selected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                SelectEntry(entry);
            }
            if (ImGui::IsItemVisible())
            {
                ImGui::SameLine(0.0f, 8.0f);
                RenderEntryLabel(entry);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                OpenEntry(entry);
                if (!entry.is_directory && mMode == Mode::OpenFile)
                {
                    ImGui::CloseCurrentPopup();
                    changed = true;
                }
            }
            ImGui::PopID();
        }
    }

    ImGui::EndChild();

    if (mMode != Mode::ChooseFolder)
    {
        ImGui::SetNextItemWidth(760.0f);
        ImGui::InputTextWithHint(
            "##dialog_file_name",
            mMode == Mode::OpenFile ? "File to open" : "File to save",
            &mFileName);
        if (mFocusFilename)
        {
            ImGui::SetKeyboardFocusHere(-1);
            mFocusFilename = false;
        }
    }

    const bool has_target =
        mMode == Mode::ChooseFolder ? !mCurrentDirectory.empty() : !mFileName.empty();
    if (!has_target)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(ConfirmLabel(mMode).c_str(), ImVec2(100.0f, 0.0f)))
    {
        AcceptCurrentSelection();
        ImGui::CloseCurrentPopup();
        changed = true;
    }
    if (!has_target)
    {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
    {
        Cancel();
        ImGui::CloseCurrentPopup();
        changed = true;
    }

    ImGui::EndPopup();
    return changed;
}
