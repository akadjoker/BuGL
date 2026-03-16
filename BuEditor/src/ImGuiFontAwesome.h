#pragma once

#include "imgui.h"
#include "vendor/fontawesome/fa_solid_900_data.h"

namespace ImGuiFontAwesome
{
inline constexpr const char* kChevronUp = u8"\uf077";
inline constexpr const char* kFile = u8"\uf15b";
inline constexpr const char* kFileCode = u8"\uf1c9";
inline constexpr const char* kFileLines = u8"\uf15c";
inline constexpr const char* kFloppyDisk = u8"\uf0c7";
inline constexpr const char* kFolder = u8"\uf07b";
inline constexpr const char* kFolderOpen = u8"\uf07c";
inline constexpr const char* kGears = u8"\uf085";
inline constexpr const char* kPlay = u8"\uf04b";
inline constexpr const char* kTerminal = u8"\uf120";

inline bool gSolidLoaded = false;

inline bool MergeSolid(ImGuiIO& io, ImFont* dst_font, float icon_size_pixels = 13.0f)
{
    static const ImWchar icon_ranges[] = {
        0xf04b, 0xf04b,
        0xf077, 0xf07c,
        0xf085, 0xf085,
        0xf0c7, 0xf0c7,
        0xf120, 0xf120,
        0xf15b, 0xf15c,
        0xf1c9, 0xf1c9,
        0,
    };

    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.FontDataOwnedByAtlas = false;
    config.DstFont = dst_font;

    gSolidLoaded = io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(BuEditorFontAwesomeData::BuEditor_src_vendor_fontawesome_fa_solid_900_otf),
        static_cast<int>(BuEditorFontAwesomeData::BuEditor_src_vendor_fontawesome_fa_solid_900_otf_len),
        icon_size_pixels,
        &config,
        icon_ranges) != nullptr;
    return gSolidLoaded;
}

inline bool MergeSolid(ImGuiIO& io, float icon_size_pixels = 13.0f)
{
    return MergeSolid(io, nullptr, icon_size_pixels);
}

inline bool IsLoaded()
{
    return gSolidLoaded;
}
}
