#include "ImGuiConsole.h"
#include "imgui.h"

#include <cstdlib>
#include <sstream>

namespace
{
struct ConsoleStyle
{
    ImVec4 color = ImVec4(0.82f, 0.84f, 0.88f, 1.0f);
    bool bright = false;
};

ImVec4 GetAnsiColor(int code, bool bright)
{
    const bool use_bright = bright || code >= 90;
    const int base = code >= 90 ? code - 90 : code - 30;
    static const ImVec4 normal_colors[8] = {
        ImVec4(0.07f, 0.07f, 0.07f, 1.0f),
        ImVec4(0.80f, 0.29f, 0.29f, 1.0f),
        ImVec4(0.38f, 0.72f, 0.36f, 1.0f),
        ImVec4(0.82f, 0.71f, 0.33f, 1.0f),
        ImVec4(0.35f, 0.55f, 0.88f, 1.0f),
        ImVec4(0.74f, 0.42f, 0.78f, 1.0f),
        ImVec4(0.31f, 0.75f, 0.78f, 1.0f),
        ImVec4(0.82f, 0.84f, 0.88f, 1.0f)
    };
    static const ImVec4 bright_colors[8] = {
        ImVec4(0.35f, 0.37f, 0.40f, 1.0f),
        ImVec4(0.98f, 0.48f, 0.45f, 1.0f),
        ImVec4(0.55f, 0.88f, 0.52f, 1.0f),
        ImVec4(0.98f, 0.86f, 0.46f, 1.0f),
        ImVec4(0.50f, 0.70f, 0.98f, 1.0f),
        ImVec4(0.89f, 0.59f, 0.92f, 1.0f),
        ImVec4(0.49f, 0.90f, 0.92f, 1.0f),
        ImVec4(0.96f, 0.97f, 0.98f, 1.0f)
    };

    if (base < 0 || base > 7)
    {
        return normal_colors[7];
    }

    return use_bright ? bright_colors[base] : normal_colors[base];
}

void ApplyAnsiCode(int code, ConsoleStyle& style, const ImVec4& default_color)
{
    if (code == 0)
    {
        style.color = default_color;
        style.bright = false;
    }
    else if (code == 1)
    {
        style.bright = true;
    }
    else if (code == 22)
    {
        style.bright = false;
    }
    else if (code == 39)
    {
        style.color = default_color;
    }
    else if ((code >= 30 && code <= 37) || (code >= 90 && code <= 97))
    {
        style.color = GetAnsiColor(code, style.bright);
    }
}
}

void ImGuiConsole::SetText(const std::string& text)
{
    mText = text;
}

const std::string& ImGuiConsole::GetText() const
{
    return mText;
}

void ImGuiConsole::Clear()
{
    mText.clear();
}

void ImGuiConsole::SetVisible(bool visible)
{
    mVisible = visible;
}

bool ImGuiConsole::IsVisible() const
{
    return mVisible;
}

ImGuiConsole::RenderResult ImGuiConsole::Render(const char* title,
                                                const char* subtitle,
                                                bool running,
                                                float height)
{
    RenderResult result;
    if (!mVisible)
    {
        return result;
    }

    ImGui::Separator();
    ImGui::Text("%s%s", title ? title : "Console", running ? " [running]" : "");
    if (subtitle && subtitle[0] != '\0')
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", subtitle);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Hide"))
    {
        mVisible = false;
        result.hidden = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear"))
    {
        Clear();
        result.cleared = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy"))
    {
        ImGui::SetClipboardText(mText.c_str());
        result.copied = true;
    }

    ImGui::BeginChild("##imgui_console_output", ImVec2(-1.0f, height), true, ImGuiWindowFlags_HorizontalScrollbar);
    if (mText.empty())
    {
        ImGui::TextUnformatted("No messages.");
    }
    else
    {
        const bool should_follow_output =
            running ||
            ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeightWithSpacing() * 2.0f;
        RenderAnsiText(mText);
        if (should_follow_output)
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    return result;
}

void ImGuiConsole::RenderAnsiText(const std::string& text)
{
    const ImVec4 default_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    ConsoleStyle style;
    style.color = default_color;

    std::string segment;
    bool line_has_content = false;

    auto flush_segment = [&]()
    {
        if (segment.empty())
        {
            return;
        }

        if (line_has_content)
        {
            ImGui::SameLine(0.0f, 0.0f);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, style.color);
        ImGui::TextUnformatted(segment.c_str());
        ImGui::PopStyleColor();
        segment.clear();
        line_has_content = true;
    };

    for (size_t i = 0; i < text.size();)
    {
        if (text[i] == '\x1b' && (i + 1) < text.size() && text[i + 1] == '[')
        {
            flush_segment();
            size_t j = i + 2;
            std::string code_text;
            while (j < text.size() && text[j] != 'm')
            {
                code_text += text[j];
                ++j;
            }

            if (j < text.size() && text[j] == 'm')
            {
                if (code_text.empty())
                {
                    ApplyAnsiCode(0, style, default_color);
                }
                else
                {
                    std::stringstream code_stream(code_text);
                    std::string item;
                    while (std::getline(code_stream, item, ';'))
                    {
                        ApplyAnsiCode(item.empty() ? 0 : std::atoi(item.c_str()), style, default_color);
                    }
                }
                i = j + 1;
                continue;
            }
        }

        if (text[i] == '\r')
        {
            ++i;
            continue;
        }

        if (text[i] == '\n')
        {
            if (!segment.empty())
            {
                flush_segment();
            }
            else if (!line_has_content)
            {
                ImGui::TextUnformatted("");
            }
            line_has_content = false;
            ++i;
            continue;
        }

        segment += text[i];
        ++i;
    }

    flush_segment();
}
