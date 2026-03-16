#pragma once

#include "imgui.h"

#include <functional>
#include <string>

class ImGuiPopup
{
public:
    enum class Kind
    {
        Info,
        Warning,
        Error,
        Confirm
    };

    void Open(Kind kind,
              const std::string& title,
              const std::string& message,
              std::function<void()> on_confirm = {},
              std::function<void()> on_cancel = {});

    bool Render();

private:
    const char* ConfirmLabel() const;
    ImVec4 AccentColor() const;

    bool mOpenRequested = false;
    Kind mKind = Kind::Info;
    std::string mTitle;
    std::string mMessage;
    std::function<void()> mOnConfirm;
    std::function<void()> mOnCancel;
};
