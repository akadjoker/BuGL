#pragma once

#include "imgui.h"
#include "ImGuiFontAwesome.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

struct FileNode
{
    std::string name;
    std::string full_path;
    bool is_directory = false;
    bool expanded = false;
    bool scanned = false;
    std::vector<FileNode> children;
};

namespace ImGuiFileExplorer
{

inline void ScanDirectory(FileNode& node)
{
    if (node.scanned || !node.is_directory)
    {
        return;
    }
    node.scanned = true;
    node.children.clear();

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(node.full_path, ec))
    {
        if (ec)
        {
            break;
        }

        const std::string name = entry.path().filename().string();
        if (name.empty() || name[0] == '.')
        {
            continue;
        }

        FileNode child;
        child.name = name;
        child.full_path = entry.path().string();
        child.is_directory = entry.is_directory(ec);

        if (!child.is_directory)
        {
            const std::string ext = entry.path().extension().string();
            if (ext != ".bu" && ext != ".json" && ext != ".md" && ext != ".txt")
            {
                continue;
            }
        }

        // Skip build, vendor, bin directories
        if (child.is_directory && (child.name == "build" || child.name == ".git" || child.name == "bin"))
        {
            continue;
        }

        node.children.push_back(std::move(child));
    }

    std::sort(node.children.begin(), node.children.end(), [](const FileNode& a, const FileNode& b)
    {
        if (a.is_directory != b.is_directory)
        {
            return a.is_directory > b.is_directory;
        }
        return a.name < b.name;
    });
}

inline void Rescan(FileNode& node)
{
    node.scanned = false;
    node.children.clear();
    ScanDirectory(node);
    for (auto& child : node.children)
    {
        if (child.is_directory && child.expanded)
        {
            Rescan(child);
        }
    }
}

inline void RenderTree(FileNode& node, const std::string& current_file,
                       const std::function<void(const std::string&)>& on_file_click)
{
    for (auto& child : node.children)
    {
        if (child.is_directory)
        {
            ImGui::PushID(child.full_path.c_str());
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (child.expanded)
            {
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            }
            const std::string icon = child.expanded ? ImGuiFontAwesome::kFolderOpen : ImGuiFontAwesome::kFolder;
            const std::string label = icon + " " + child.name;
            bool open = ImGui::TreeNodeEx("##dir", flags, "%s", label.c_str());
            if (open != child.expanded)
            {
                child.expanded = open;
            }
            if (open)
            {
                if (!child.scanned)
                {
                    ScanDirectory(child);
                }
                RenderTree(child, current_file, on_file_click);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        else
        {
            const bool is_current = child.full_path == current_file;
            ImGui::PushID(child.full_path.c_str());
            const std::string label = std::string(ImGuiFontAwesome::kFileCode) + " " + child.name;
            if (ImGui::Selectable(label.c_str(), is_current))
            {
                if (on_file_click)
                {
                    on_file_click(child.full_path);
                }
            }
            ImGui::PopID();
        }
    }
}

inline void Render(const char* id, FileNode& root, float width, float height,
                   const std::string& current_file,
                   const std::function<void(const std::string&)>& on_file_click)
{
    ImGui::BeginChild(id, ImVec2(width, height), true);
    ImGui::TextUnformatted(ImGuiFontAwesome::kFolder);
    ImGui::SameLine();
    ImGui::TextUnformatted("Explorer");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.0f);
    if (ImGui::SmallButton("R"))
    {
        Rescan(root);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Refresh");
    }
    ImGui::Separator();

    if (!root.scanned)
    {
        ScanDirectory(root);
    }

    RenderTree(root, current_file, on_file_click);
    ImGui::EndChild();
}

} // namespace ImGuiFileExplorer
