#pragma once

#include <string>

class ImGuiConsole
{
public:
    struct RenderResult
    {
        bool cleared = false;
        bool copied = false;
        bool hidden = false;
    };

    void SetText(const std::string& text);
    const std::string& GetText() const;

    void Clear();

    void SetVisible(bool visible);
    bool IsVisible() const;

    RenderResult Render(const char* title,
                        const char* subtitle,
                        bool running,
                        float height);

private:
    static void RenderAnsiText(const std::string& text);

    bool mVisible = false;
    std::string mText;
};
