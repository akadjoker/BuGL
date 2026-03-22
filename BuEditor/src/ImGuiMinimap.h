#pragma once

#include "imgui.h"
#include "TextEditor.h"

namespace ImGuiMinimap
{

inline void Render(const char* id, TextEditor& editor, float width, float height, ImFont* font)
{
    ImGui::BeginChild(id, ImVec2(width, height), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 region = ImGui::GetContentRegionAvail();

    const int total_lines = editor.GetLineCount();
    if (total_lines <= 0)
    {
        ImGui::EndChild();
        return;
    }

    const float line_height = std::max(1.0f, std::min(3.0f, region.y / (float)total_lines));
    const float char_width = 1.2f;
    const int max_chars = (int)(region.x / char_width);
    const float total_map_height = line_height * total_lines;

    // Scroll offset for minimap
    float scroll_offset = 0.0f;
    if (total_map_height > region.y)
    {
        const int first_visible = editor.GetFirstVisibleLine();
        const float progress = (float)first_visible / (float)std::max(1, total_lines - 1);
        scroll_offset = progress * (total_map_height - region.y);
    }

    // Draw minimap lines
    const int start_line = std::max(0, (int)(scroll_offset / line_height) - 1);
    const int end_line = std::min(total_lines, start_line + (int)(region.y / line_height) + 2);

    for (int line = start_line; line < end_line; ++line)
    {
        const float y = origin.y + (line * line_height) - scroll_offset;
        if (y + line_height < origin.y || y > origin.y + region.y)
        {
            continue;
        }

        const int line_len = editor.GetLineLengthRaw(line);
        const int chars_to_draw = std::min(line_len, max_chars);

        // Sample colors at intervals for performance
        const int step = std::max(1, chars_to_draw / 40);
        for (int col = 0; col < chars_to_draw; col += step)
        {
            const ImU32 color = editor.GetLineGlyphColor(line, col);
            // Dim the color for minimap appearance
            const ImU32 dimmed = (color & 0x00FFFFFF) | 0xCC000000;
            const float x = origin.x + col * char_width;
            const float block_width = std::min(char_width * step, region.x - (col * char_width));
            draw_list->AddRectFilled(
                ImVec2(x, y),
                ImVec2(x + block_width, y + line_height),
                dimmed);
        }
    }

    // Draw viewport highlight
    const int first_visible = editor.GetFirstVisibleLine();
    const int last_visible = editor.GetLastVisibleLine();
    const float viewport_y_start = origin.y + (first_visible * line_height) - scroll_offset;
    const float viewport_y_end = origin.y + ((last_visible + 1) * line_height) - scroll_offset;
    draw_list->AddRectFilled(
        ImVec2(origin.x, std::max(viewport_y_start, origin.y)),
        ImVec2(origin.x + region.x, std::min(viewport_y_end, origin.y + region.y)),
        IM_COL32(255, 255, 255, 30));
    draw_list->AddRect(
        ImVec2(origin.x, std::max(viewport_y_start, origin.y)),
        ImVec2(origin.x + region.x, std::min(viewport_y_end, origin.y + region.y)),
        IM_COL32(255, 255, 255, 60));

    // Click to navigate
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const ImVec2 mouse = ImGui::GetMousePos();
        const float relative_y = mouse.y - origin.y + scroll_offset;
        const int target_line = std::clamp((int)(relative_y / line_height), 0, total_lines - 1);
        editor.SetViewAtLine(target_line, TextEditor::SetViewAtLineMode::Centered);
        editor.SetCursorPosition(target_line, 0);
    }

    // Drag to scroll
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        const ImVec2 mouse = ImGui::GetMousePos();
        const float relative_y = mouse.y - origin.y + scroll_offset;
        const int target_line = std::clamp((int)(relative_y / line_height), 0, total_lines - 1);
        editor.SetViewAtLine(target_line, TextEditor::SetViewAtLineMode::Centered);
    }

    ImGui::EndChild();
}

} // namespace ImGuiMinimap
