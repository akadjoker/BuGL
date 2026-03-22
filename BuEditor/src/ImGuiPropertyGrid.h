#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"

#include <cfloat>
#include <string>

namespace ImGuiPropertyGrid
{
inline bool Begin(const char* id, float label_width = 120.0f)
{
    if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_PadOuterX))
    {
        return false;
    }
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, label_width);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    return true;
}

inline void End()
{
    ImGui::EndTable();
}

inline bool Section(const char* label, bool default_open = true)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (default_open)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    return ImGui::CollapsingHeader(label, flags);
}

inline void Label(const char* label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);
}

inline bool InputText(const char* label, std::string* value)
{
    Label(label);
    return ImGui::InputText(("##" + std::string(label)).c_str(), value);
}

inline bool DragFloat(const char* label, float* value, float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char* format = "%.2f")
{
    Label(label);
    return ImGui::DragFloat(("##" + std::string(label)).c_str(), value, speed, min, max, format);
}

inline bool DragFloat2(const char* label, float value[2], float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char* format = "%.2f")
{
    Label(label);
    return ImGui::DragFloat2(("##" + std::string(label)).c_str(), value, speed, min, max, format);
}

inline bool DragInt(const char* label, int* value, float speed = 1.0f, int min = 0, int max = 0, const char* format = "%d")
{
    Label(label);
    return ImGui::DragInt(("##" + std::string(label)).c_str(), value, speed, min, max, format);
}

inline bool SliderFloat(const char* label, float* value, float min, float max, const char* format = "%.2f")
{
    Label(label);
    return ImGui::SliderFloat(("##" + std::string(label)).c_str(), value, min, max, format);
}

inline bool Combo(const char* label, int* current_item, const char* const items[], int items_count)
{
    Label(label);
    return ImGui::Combo(("##" + std::string(label)).c_str(), current_item, items, items_count);
}

inline bool Checkbox(const char* label, bool* value)
{
    Label(label);
    return ImGui::Checkbox(("##" + std::string(label)).c_str(), value);
}

inline bool ColorEdit4(const char* label, float value[4])
{
    Label(label);
    return ImGui::ColorEdit4(("##" + std::string(label)).c_str(), value, ImGuiColorEditFlags_DisplayRGB);
}
}
