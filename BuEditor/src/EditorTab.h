#pragma once

#include "DocumentSymbols.h"
#include "TextEditor.h"

#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>

struct EditorTab
{
    TextEditor editor;
    std::string file_path;
    std::string last_saved_text;
    std::string last_seen_text;
    std::deque<DocumentSymbol> outline_symbols;
    uint32_t last_edit_ticks = 0;
    uint32_t last_autosave_ticks = 0;

    bool IsDirty() const
    {
        return editor.GetText() != last_saved_text;
    }

    std::string GetTitle() const
    {
        if (file_path.empty())
        {
            return "Untitled";
        }
        return std::filesystem::path(file_path).filename().string();
    }

    std::string GetDisplayTitle() const
    {
        std::string title = GetTitle();
        if (IsDirty())
        {
            title += " *";
        }
        return title;
    }
};
