#pragma once

#include "imgui.h"

namespace ImGuiSplitter
{
inline void Vertical(const char* id, float* left_width, float min_left_width, float min_right_width, float height = -1.0f)
{
    ImGui::PushID(id);
    ImGui::InvisibleButton("##vertical_splitter", ImVec2(6.0f, height));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActive())
    {
        *left_width += ImGui::GetIO().MouseDelta.x;
        const float available = ImGui::GetContentRegionAvail().x + 6.0f;
        if (*left_width < min_left_width)
        {
            *left_width = min_left_width;
        }
        if (*left_width > available - min_right_width)
        {
            *left_width = available - min_right_width;
        }
    }

    const ImU32 color = ImGui::GetColorU32(
        ImGui::IsItemActive() ? ImGuiCol_SeparatorActive :
        ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered :
        ImGuiCol_Separator);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 2.0f);
    ImGui::PopID();
}

inline void VerticalRight(const char* id, float* right_width, float min_left_width, float min_right_width, float height = -1.0f)
{
    ImGui::PushID(id);
    ImGui::InvisibleButton("##vertical_splitter_right", ImVec2(6.0f, height));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActive())
    {
        *right_width -= ImGui::GetIO().MouseDelta.x;
        const float available = ImGui::GetContentRegionAvail().x + 6.0f;
        if (*right_width < min_right_width)
        {
            *right_width = min_right_width;
        }
        if (*right_width > available - min_left_width)
        {
            *right_width = available - min_left_width;
        }
    }

    const ImU32 color = ImGui::GetColorU32(
        ImGui::IsItemActive() ? ImGuiCol_SeparatorActive :
        ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered :
        ImGuiCol_Separator);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 2.0f);
    ImGui::PopID();
}

inline void Horizontal(const char* id, float* top_height, float min_top_height, float min_bottom_height, float width = -1.0f)
{
    ImGui::PushID(id);
    ImGui::InvisibleButton("##horizontal_splitter", ImVec2(width, 6.0f));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    if (ImGui::IsItemActive())
    {
        *top_height += ImGui::GetIO().MouseDelta.y;
        const float available = ImGui::GetContentRegionAvail().y + 6.0f;
        if (*top_height < min_top_height)
        {
            *top_height = min_top_height;
        }
        if (*top_height > available - min_bottom_height)
        {
            *top_height = available - min_bottom_height;
        }
    }

    const ImU32 color = ImGui::GetColorU32(
        ImGui::IsItemActive() ? ImGuiCol_SeparatorActive :
        ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered :
        ImGuiCol_Separator);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 2.0f);
    ImGui::PopID();
}
}
