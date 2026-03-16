#include "ImGuiPopup.h"

void ImGuiPopup::Open(Kind kind,
                      const std::string& title,
                      const std::string& message,
                      std::function<void()> on_confirm,
                      std::function<void()> on_cancel)
{
    mKind = kind;
    mTitle = title;
    mMessage = message;
    mOnConfirm = std::move(on_confirm);
    mOnCancel = std::move(on_cancel);
    mOpenRequested = true;
}

const char* ImGuiPopup::ConfirmLabel() const
{
    switch (mKind)
    {
    case Kind::Info: return "OK";
    case Kind::Warning: return "OK";
    case Kind::Error: return "Close";
    case Kind::Confirm: return "Confirm";
    }
    return "OK";
}

ImVec4 ImGuiPopup::AccentColor() const
{
    switch (mKind)
    {
    case Kind::Info: return ImVec4(0.33f, 0.68f, 0.93f, 1.0f);
    case Kind::Warning: return ImVec4(0.91f, 0.73f, 0.24f, 1.0f);
    case Kind::Error: return ImVec4(0.88f, 0.33f, 0.30f, 1.0f);
    case Kind::Confirm: return ImVec4(0.39f, 0.75f, 0.48f, 1.0f);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Button);
}

bool ImGuiPopup::Render()
{
    bool changed = false;

    if (mOpenRequested)
    {
        ImGui::OpenPopup(mTitle.c_str());
        mOpenRequested = false;
    }

    if (!ImGui::BeginPopupModal(mTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return changed;
    }

    const ImVec4 accent = AccentColor();
    ImGui::PushStyleColor(ImGuiCol_Text, accent);
    ImGui::TextUnformatted(mTitle.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::PushTextWrapPos(420.0f);
    ImGui::TextUnformatted(mMessage.c_str());
    ImGui::PopTextWrapPos();
    ImGui::Spacing();

    const bool is_confirm = mKind == Kind::Confirm;
    if (ImGui::Button(ConfirmLabel(), ImVec2(110.0f, 0.0f)))
    {
        if (mOnConfirm)
        {
            mOnConfirm();
        }
        ImGui::CloseCurrentPopup();
        changed = true;
    }

    if (is_confirm)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
        {
            if (mOnCancel)
            {
                mOnCancel();
            }
            ImGui::CloseCurrentPopup();
            changed = true;
        }
    }

    ImGui::EndPopup();
    return changed;
}
