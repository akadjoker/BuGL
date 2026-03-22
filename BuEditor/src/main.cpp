#include "DocumentSymbols.h"
#include "DejaVuSansMono_embedded.h"
#include "EditorTab.h"
#include "GifRecorder.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileExplorer.h"
#include "ImGuiConsole.h"
#include "ImGuiFontAwesome.h"
#include "ImGuiMinimap.h"
#include "ImGuiSplitter.h"
#include "SnippetManager.h"
#include "TextEditor.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_stdlib.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace
{
using json = nlohmann::json;

constexpr const char* kBuEditorVersion = "0.2.0";
constexpr const char* kBuEditorBuildDate = __DATE__;

enum class EditorFontChoice
{
    Default,
    DroidSans,
    Custom
};

enum class OutlineSide
{
    Left,
    Right
};

struct EditorFontEntry
{
    std::string id;
    std::string label;
    ImFont* font = nullptr;
};

struct EditorSettings
{
    bool autosave_enabled = false;
    int autosave_interval_ms = 1500;
    bool console_visible = false;
    TextEditor::PaletteId palette = TextEditor::PaletteId::VsCodeDark;
    EditorFontChoice font_choice = EditorFontChoice::Default;
    std::string font_path;
    float font_scale = 1.0f;
    std::string last_file_path = "scripts/vm_cheatsheet.bu";
    std::string bugl_path;
    std::string bytecode_output_path;
    bool outline_visible = true;
    OutlineSide outline_side = OutlineSide::Right;
    bool file_explorer_visible = true;
    bool minimap_visible = false;
};

std::string gSettingsPath;
std::string gLegacySettingsPath;
std::string gProjectDir;

struct AsyncCommandState
{
    std::mutex mutex;
    bool running = false;
    bool has_result = false;
    int exit_code = -1;
    std::string label;
    std::string command;
    std::string output;
};

struct FindReplaceState
{
    bool visible = false;
    bool replace_visible = false;
    bool case_sensitive = true;
    bool focus_find_input = false;
    bool focus_replace_input = false;
    std::string find_text;
    std::string replace_text;
};

struct GoToLineState
{
    bool visible = false;
    bool focus_input = false;
    std::string line_text;
};

struct FontPickerState
{
    bool visible = false;
    bool focus_filter = false;
    std::string filter;
};

struct OutlineState
{
    bool visible = true;
    float width = 220.0f;
    OutlineSide side = OutlineSide::Left;
};

struct FileExplorerState
{
    bool visible = true;
    float width = 200.0f;
    FileNode root;
};

struct MinimapState
{
    bool visible = false;
    float width = 80.0f;
};

struct SnippetPopupState
{
    bool visible = false;
    bool focus_filter = false;
    std::string filter;
};

struct UnsavedCloseState
{
    bool visible = false;
    int tab_index = -1;   // -1 means closing app
    bool closing_app = false;
};

std::vector<size_t> BuildLineOffsets(const std::string& text)
{
    std::vector<size_t> offsets;
    offsets.push_back(0);
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\n')
        {
            offsets.push_back(i + 1);
        }
    }
    return offsets;
}

size_t LineColumnToOffset(const std::vector<size_t>& line_offsets, int line, int column)
{
    if (line_offsets.empty())
    {
        return 0;
    }
    const int clamped_line = std::max(0, std::min(line, static_cast<int>(line_offsets.size()) - 1));
    return line_offsets[clamped_line] + static_cast<size_t>(std::max(0, column));
}

TextEditor::TextPosition OffsetToTextPosition(const std::vector<size_t>& line_offsets, size_t offset)
{
    TextEditor::TextPosition result;
    if (line_offsets.empty())
    {
        return result;
    }

    auto it = std::upper_bound(line_offsets.begin(), line_offsets.end(), offset);
    const int line = static_cast<int>(std::max<std::ptrdiff_t>(0, (it - line_offsets.begin()) - 1));
    result.line = line;
    result.column = static_cast<int>(offset - line_offsets[line]);
    return result;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch)
    {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

bool FindTextRange(const std::string& text,
                   const std::string& needle,
                   bool case_sensitive,
                   size_t start_offset,
                   bool forward,
                   size_t& out_start,
                   size_t& out_end)
{
    if (needle.empty())
    {
        return false;
    }

    const std::string haystack = case_sensitive ? text : ToLowerCopy(text);
    const std::string token = case_sensitive ? needle : ToLowerCopy(needle);

    if (forward)
    {
        size_t found = haystack.find(token, std::min(start_offset, haystack.size()));
        if (found == std::string::npos && start_offset > 0)
        {
            found = haystack.find(token, 0);
        }
        if (found == std::string::npos)
        {
            return false;
        }
        out_start = found;
        out_end = found + token.size();
        return true;
    }

    size_t found = std::string::npos;
    if (!haystack.empty())
    {
        const size_t bounded_start = std::min(start_offset, haystack.size());
        if (bounded_start > 0)
        {
            found = haystack.rfind(token, bounded_start - 1);
        }
        if (found == std::string::npos)
        {
            found = haystack.rfind(token);
        }
    }
    if (found == std::string::npos)
    {
        return false;
    }
    out_start = found;
    out_end = found + token.size();
    return true;
}

bool SelectionMatchesQuery(const std::string& selected_text, const std::string& query, bool case_sensitive)
{
    return case_sensitive ? selected_text == query : ToLowerCopy(selected_text) == ToLowerCopy(query);
}

const char* SymbolIcon(DocumentSymbolKind kind)
{
    switch (kind)
    {
    case DocumentSymbolKind::Class: return ImGuiFontAwesome::kGears;
    case DocumentSymbolKind::Struct: return ImGuiFontAwesome::kFileLines;
    case DocumentSymbolKind::Def: return ImGuiFontAwesome::kPlay;
    case DocumentSymbolKind::Process: return ImGuiFontAwesome::kTerminal;
    }
    return ImGuiFontAwesome::kFile;
}

const DocumentSymbol* FindInnermostSymbolAtLine(const std::deque<DocumentSymbol>& symbols, int line)
{
    for (const DocumentSymbol& symbol : symbols)
    {
        if (line < symbol.line || line > symbol.end_line)
        {
            continue;
        }

        if (const DocumentSymbol* child = FindInnermostSymbolAtLine(symbol.children, line))
        {
            return child;
        }
        return &symbol;
    }
    return nullptr;
}

std::string BuildBreadcrumbPath(const std::deque<DocumentSymbol>& symbols, int line)
{
    std::string path;
    const std::deque<DocumentSymbol>* current = &symbols;
    while (current)
    {
        bool found = false;
        for (const DocumentSymbol& symbol : *current)
        {
            if (line >= symbol.line && line <= symbol.end_line)
            {
                if (!path.empty()) path += " > ";
                path += symbol.name;
                current = &symbol.children;
                found = true;
                break;
            }
        }
        if (!found) break;
    }
    return path;
}

bool FileExists(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    return input.good();
}

std::string GetDefaultBuglPath()
{
#if defined(_WIN32)
    return "bugl.exe";
#else
    return "bugl";
#endif
}

std::string GetDefaultBytecodePath(const std::string& script_path)
{
    if (script_path.empty())
    {
        return "scripts/main.buc";
    }

    std::filesystem::path path(script_path);
    path.replace_extension(".buc");
    return path.lexically_normal().string();
}

std::filesystem::path GetExecutableDirectory(const char* argv0)
{
    std::error_code ec;
    if (argv0 != nullptr && argv0[0] != '\0')
    {
        const std::filesystem::path absolute_path = std::filesystem::absolute(argv0, ec);
        if (!ec && !absolute_path.empty())
        {
            return absolute_path.parent_path().lexically_normal();
        }
    }

    return std::filesystem::current_path(ec).lexically_normal();
}

std::string NormalizePath(const std::filesystem::path& path)
{
    std::error_code ec;
    const std::filesystem::path absolute_path = std::filesystem::absolute(path, ec);
    if (!ec)
    {
        return absolute_path.lexically_normal().string();
    }

    return path.lexically_normal().string();
}

// Convert an absolute path to relative (relative to project dir) for portable storage
std::string ToRelativePath(const std::string& abs_path)
{
    if (abs_path.empty() || gProjectDir.empty()) return abs_path;
    std::filesystem::path p(abs_path);
    std::filesystem::path base(gProjectDir);
    // Only relativize if the path is inside the project directory
    auto rel = p.lexically_relative(base);
    if (!rel.empty() && rel.string().substr(0, 2) != "..")
        return rel.string();
    return abs_path; // keep absolute if outside project dir (e.g. system font)
}

// Resolve a potentially relative path back to absolute (relative to project dir)
std::string ToAbsolutePath(const std::string& rel_path)
{
    if (rel_path.empty() || gProjectDir.empty()) return rel_path;
    std::filesystem::path p(rel_path);
    if (p.is_absolute()) return rel_path;
    return NormalizePath(std::filesystem::path(gProjectDir) / p);
}

std::string ResolveExistingPath(const std::string& value, const std::filesystem::path& executable_dir)
{
    if (value.empty())
    {
        return value;
    }

    const std::filesystem::path input_path(value);
    const std::filesystem::path project_dir = executable_dir.parent_path();
    std::vector<std::filesystem::path> candidates;

    if (input_path.is_absolute())
    {
        candidates.push_back(input_path);
    }
    else
    {
        candidates.push_back(std::filesystem::current_path() / input_path);
        candidates.push_back(executable_dir / input_path);
        candidates.push_back(project_dir / input_path);
    }

    for (const std::filesystem::path& candidate : candidates)
    {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec))
        {
            return NormalizePath(candidate);
        }
    }

    return value;
}

std::string ResolveWritablePath(const std::string& value, const std::filesystem::path& executable_dir)
{
    if (value.empty())
    {
        return value;
    }

    const std::filesystem::path input_path(value);
    if (input_path.is_absolute())
    {
        return NormalizePath(input_path);
    }

    return NormalizePath(executable_dir.parent_path() / input_path);
}

std::filesystem::path GetProjectDirectory(const std::filesystem::path& executable_dir)
{
    return executable_dir.parent_path().empty() ? executable_dir : executable_dir.parent_path();
}

std::string QuoteCommandArgument(const std::string& value)
{
    std::string quoted = "\"";
    for (char ch : value)
    {
        if (ch == '"')
        {
            quoted += "\\\"";
        }
        else
        {
            quoted += ch;
        }
    }
    quoted += '"';
    return quoted;
}

std::string BuildShellCommand(const std::filesystem::path& working_directory, const std::string& command)
{
#if defined(_WIN32)
    return "cd /d " + QuoteCommandArgument(working_directory.string()) + " && " + command;
#else
    return "cd " + QuoteCommandArgument(working_directory.string()) + " && " + command;
#endif
}

FILE* OpenPipe(const char* command, const char* mode)
{
#if defined(_WIN32)
    return _popen(command, mode);
#else
    return popen(command, mode);
#endif
}

int ClosePipe(FILE* pipe)
{
#if defined(_WIN32)
    return _pclose(pipe);
#else
    const int raw_code = pclose(pipe);
    if (raw_code == -1)
    {
        return -1;
    }
    if (WIFEXITED(raw_code))
    {
        return WEXITSTATUS(raw_code);
    }
    return raw_code;
#endif
}

const char* PaletteIdToString(TextEditor::PaletteId palette)
{
    switch (palette)
    {
    case TextEditor::PaletteId::Dark: return "dark";
    case TextEditor::PaletteId::Light: return "light";
    case TextEditor::PaletteId::Mariana: return "mariana";
    case TextEditor::PaletteId::RetroBlue: return "retro_blue";
    case TextEditor::PaletteId::VsCodeDark: return "vscode_dark";
    }
    return "vscode_dark";
}

TextEditor::PaletteId PaletteIdFromString(const std::string& value)
{
    if (value == "dark") return TextEditor::PaletteId::Dark;
    if (value == "light") return TextEditor::PaletteId::Light;
    if (value == "mariana") return TextEditor::PaletteId::Mariana;
    if (value == "retro_blue") return TextEditor::PaletteId::RetroBlue;
    return TextEditor::PaletteId::VsCodeDark;
}

const char* FontChoiceToString(EditorFontChoice font_choice)
{
    switch (font_choice)
    {
    case EditorFontChoice::Default: return "default";
    case EditorFontChoice::DroidSans: return "droid_sans";
    case EditorFontChoice::Custom: return "custom";
    }
    return "default";
}

EditorFontChoice FontChoiceFromString(const std::string& value)
{
    if (value == "droid_sans")
    {
        return EditorFontChoice::DroidSans;
    }
    if (value == "custom")
    {
        return EditorFontChoice::Custom;
    }
    return EditorFontChoice::Default;
}

const char* OutlineSideToString(OutlineSide side)
{
    switch (side)
    {
    case OutlineSide::Left: return "left";
    case OutlineSide::Right: return "right";
    }
    return "left";
}

OutlineSide OutlineSideFromString(const std::string& value)
{
    if (value == "right")
    {
        return OutlineSide::Right;
    }
    return OutlineSide::Left;
}

std::string MakeFontLabel(const std::filesystem::path& path, const char* prefix)
{
    const std::string stem = path.stem().string();
    return std::string(prefix) + ": " + (stem.empty() ? path.filename().string() : stem);
}

void AppendDiscoveredFonts(std::vector<std::pair<std::string, std::string>>& out_fonts,
                           const std::filesystem::path& directory,
                           const char* prefix)
{
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
    {
        return;
    }

    for (std::filesystem::recursive_directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec), end;
         !ec && it != end;
         it.increment(ec))
    {
        if (!it->is_regular_file(ec))
        {
            continue;
        }

        std::filesystem::path path = it->path();
        std::string extension = ToLowerCopy(path.extension().string());
        if (extension != ".ttf" && extension != ".otf")
        {
            continue;
        }

        const std::string normalized = NormalizePath(path);
        const auto duplicate = std::find_if(out_fonts.begin(), out_fonts.end(), [&](const auto& item)
        {
            return item.first == normalized;
        });
        if (duplicate != out_fonts.end())
        {
            continue;
        }

        out_fonts.push_back({normalized, MakeFontLabel(path, prefix)});
    }
}

void AppendKnownSystemFonts(std::vector<std::pair<std::string, std::string>>& out_fonts,
                            const std::filesystem::path& root_directory,
                            const std::vector<std::string>& file_names)
{
    std::error_code ec;
    if (!std::filesystem::exists(root_directory, ec) || !std::filesystem::is_directory(root_directory, ec))
    {
        return;
    }

    for (const std::string& file_name : file_names)
    {
        bool found = false;
        for (std::filesystem::recursive_directory_iterator it(root_directory, std::filesystem::directory_options::skip_permission_denied, ec), end;
             !ec && it != end;
             it.increment(ec))
        {
            if (!it->is_regular_file(ec))
            {
                continue;
            }
            if (ToLowerCopy(it->path().filename().string()) != ToLowerCopy(file_name))
            {
                continue;
            }

            const std::string normalized = NormalizePath(it->path());
            const auto duplicate = std::find_if(out_fonts.begin(), out_fonts.end(), [&](const auto& item)
            {
                return item.first == normalized;
            });
            if (duplicate == out_fonts.end())
            {
                out_fonts.push_back({normalized, MakeFontLabel(it->path(), "System")});
            }
            found = true;
            break;
        }
        ec.clear();
        if (found)
        {
            continue;
        }
    }
}

std::vector<std::pair<std::string, std::string>> DiscoverEditorFonts(const std::filesystem::path& executable_dir,
                                                                     const std::filesystem::path& project_dir)
{
    std::vector<std::pair<std::string, std::string>> fonts;
    AppendDiscoveredFonts(fonts, executable_dir / "assets" / "fonts", "Assets");
    AppendDiscoveredFonts(fonts, project_dir / "assets" / "fonts", "Assets");
    AppendDiscoveredFonts(fonts, project_dir / "BuEditor" / "assets" / "fonts", "Editor");

#if defined(_WIN32)
    AppendKnownSystemFonts(fonts,
                           "C:/Windows/Fonts",
                           {"consola.ttf", "CascadiaMono.ttf", "CascadiaCode.ttf", "segoeui.ttf", "cour.ttf"});
#else
    const std::vector<std::string> system_font_names = {
        "DejaVuSansMono.ttf",
        "NotoSansMono-Regular.ttf",
        "LiberationMono-Regular.ttf",
        "UbuntuMono-R.ttf",
        "JetBrainsMono-Regular.ttf",
        "FiraCode-Regular.ttf",
    };
    AppendKnownSystemFonts(fonts, "/usr/share/fonts", system_font_names);
    AppendKnownSystemFonts(fonts, "/usr/local/share/fonts", system_font_names);
    if (const char* home = std::getenv("HOME"))
    {
        AppendKnownSystemFonts(fonts, std::filesystem::path(home) / ".fonts", system_font_names);
        AppendKnownSystemFonts(fonts, std::filesystem::path(home) / ".local" / "share" / "fonts", system_font_names);
    }
#endif

    std::sort(fonts.begin(), fonts.end(), [](const auto& lhs, const auto& rhs)
    {
        return lhs.second < rhs.second;
    });
    return fonts;
}

bool LoadSettings(EditorSettings& settings)
{
    std::ifstream input(gSettingsPath, std::ios::binary);
    if (!input.is_open() && !gLegacySettingsPath.empty())
    {
        input.open(gLegacySettingsPath, std::ios::binary);
    }
    if (!input.is_open())
    {
        return false;
    }

    json document;
    try
    {
        input >> document;
    }
    catch (const json::exception&)
    {
        return false;
    }

    if (!document.is_object())
    {
        return false;
    }

    if (document.contains("autosave_enabled") && document["autosave_enabled"].is_boolean())
    {
        settings.autosave_enabled = document["autosave_enabled"].get<bool>();
    }
    if (document.contains("autosave_interval_ms") && document["autosave_interval_ms"].is_number_integer())
    {
        settings.autosave_interval_ms = std::clamp(document["autosave_interval_ms"].get<int>(), 250, 30000);
    }
    if (document.contains("console_visible") && document["console_visible"].is_boolean())
    {
        settings.console_visible = document["console_visible"].get<bool>();
    }
    if (document.contains("palette") && document["palette"].is_string())
    {
        settings.palette = PaletteIdFromString(document["palette"].get<std::string>());
    }
    if (document.contains("font_choice") && document["font_choice"].is_string())
    {
        settings.font_choice = FontChoiceFromString(document["font_choice"].get<std::string>());
    }
    if (document.contains("font_path") && document["font_path"].is_string())
    {
        settings.font_path = document["font_path"].get<std::string>();
    }
    if (document.contains("font_scale") && document["font_scale"].is_number())
    {
        settings.font_scale = std::clamp(document["font_scale"].get<float>(), 0.6f, 2.5f);
    }
    if (document.contains("last_file_path") && document["last_file_path"].is_string())
    {
        settings.last_file_path = document["last_file_path"].get<std::string>();
    }
    if (document.contains("bugl_path") && document["bugl_path"].is_string())
    {
        settings.bugl_path = document["bugl_path"].get<std::string>();
    }
    if (document.contains("bytecode_output_path") && document["bytecode_output_path"].is_string())
    {
        settings.bytecode_output_path = document["bytecode_output_path"].get<std::string>();
    }
    if (document.contains("outline_visible") && document["outline_visible"].is_boolean())
    {
        settings.outline_visible = document["outline_visible"].get<bool>();
    }
    if (document.contains("outline_side") && document["outline_side"].is_string())
    {
        settings.outline_side = OutlineSideFromString(document["outline_side"].get<std::string>());
    }
    if (document.contains("file_explorer_visible") && document["file_explorer_visible"].is_boolean())
    {
        settings.file_explorer_visible = document["file_explorer_visible"].get<bool>();
    }
    if (document.contains("minimap_visible") && document["minimap_visible"].is_boolean())
    {
        settings.minimap_visible = document["minimap_visible"].get<bool>();
    }

    if (document.contains("runtime") && document["runtime"].is_object())
    {
        const json& runtime = document["runtime"];
        if (runtime.contains("binary_path") && runtime["binary_path"].is_string())
        {
            settings.bugl_path = runtime["binary_path"].get<std::string>();
        }
    }

    if (document.contains("workspace") && document["workspace"].is_object())
    {
        const json& workspace = document["workspace"];
        if (workspace.contains("entry_script") && workspace["entry_script"].is_string())
        {
            settings.last_file_path = workspace["entry_script"].get<std::string>();
        }
        if (workspace.contains("bytecode_output_path") && workspace["bytecode_output_path"].is_string())
        {
            settings.bytecode_output_path = workspace["bytecode_output_path"].get<std::string>();
        }
    }

    // Resolve relative paths to absolute (paths are stored relative to project root)
    settings.font_path = ToAbsolutePath(settings.font_path);
    settings.last_file_path = ToAbsolutePath(settings.last_file_path);
    settings.bugl_path = ToAbsolutePath(settings.bugl_path);
    settings.bytecode_output_path = ToAbsolutePath(settings.bytecode_output_path);

    if (settings.bugl_path.empty())
    {
        settings.bugl_path = GetDefaultBuglPath();
    }
    if (settings.bytecode_output_path.empty())
    {
        settings.bytecode_output_path = GetDefaultBytecodePath(settings.last_file_path);
    }

    return true;
}

bool SaveSettings(const EditorSettings& settings)
{
    std::error_code ec;
    if (!gSettingsPath.empty())
    {
        std::filesystem::create_directories(std::filesystem::path(gSettingsPath).parent_path(), ec);
    }

    std::ofstream output(gSettingsPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }

    // Convert absolute paths to relative (relative to project root) for portability
    const std::string rel_font_path = ToRelativePath(settings.font_path);
    const std::string rel_last_file = ToRelativePath(settings.last_file_path);
    const std::string rel_bugl = ToRelativePath(settings.bugl_path);
    const std::string rel_bytecode = ToRelativePath(settings.bytecode_output_path);

    json document = {
        {"autosave_enabled", settings.autosave_enabled},
        {"autosave_interval_ms", settings.autosave_interval_ms},
        {"console_visible", settings.console_visible},
        {"palette", PaletteIdToString(settings.palette)},
        {"font_choice", FontChoiceToString(settings.font_choice)},
        {"font_path", rel_font_path},
        {"font_scale", settings.font_scale},
        {"last_file_path", rel_last_file},
        {"bugl_path", rel_bugl},
        {"bytecode_output_path", rel_bytecode},
        {"outline_visible", settings.outline_visible},
        {"outline_side", OutlineSideToString(settings.outline_side)},
        {"file_explorer_visible", settings.file_explorer_visible},
        {"minimap_visible", settings.minimap_visible},
        {"runtime", {
            {"profile_name", "bugl"},
            {"binary_path", rel_bugl},
        }},
        {"workspace", {
            {"root_path", ""},
            {"entry_script", rel_last_file},
            {"bytecode_output_path", rel_bytecode},
        }},
    };

    output << document.dump(2) << '\n';
    return true;
}

bool LoadTextFile(const std::string& path, std::string& content, std::string& status)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
    {
        status = "Failed to open: " + path;
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    content = buffer.str();
    status = "Opened: " + path;
    return true;
}

bool SaveTextFile(const std::string& path, const std::string& content, std::string& status)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        status = "Failed to save: " + path;
        return false;
    }

    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output.good())
    {
        status = "Write error: " + path;
        return false;
    }

    status = "Saved: " + path;
    return true;
}

void UpdateWindowTitle(SDL_Window* window, const std::string& file_path, bool dirty, bool gif_recording)
{
    std::string title = "BuEditor";
    if (gif_recording)
    {
        title += " [REC]";
    }
    if (!file_path.empty())
    {
        title += " - ";
        title += file_path;
    }
    if (dirty)
    {
        title += " *";
    }
    SDL_SetWindowTitle(window, title.c_str());
}
}

int main(int argc, char** argv)
{
    const std::filesystem::path executable_dir = GetExecutableDirectory(argc > 0 ? argv[0] : nullptr);
    const std::filesystem::path project_dir = GetProjectDirectory(executable_dir);
    gProjectDir = NormalizePath(project_dir);
    gSettingsPath = NormalizePath(project_dir / "config" / "settings.json");
    gLegacySettingsPath = NormalizePath(project_dir / "settings.json");

#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    const char* glsl_version = "#version 330";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_Window* window = SDL_CreateWindow(
        "BuEditor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1440,
        900,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
    {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    const auto discovered_fonts = DiscoverEditorFonts(executable_dir, project_dir);
    std::vector<EditorFontEntry> editor_fonts;

    // Extended glyph ranges: default + Latin Extended + General Punctuation (em/en dash, ellipsis, etc.)
    static const ImWchar extended_glyph_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A (accented chars)
        0x0180, 0x024F, // Latin Extended-B
        0x2000, 0x206F, // General Punctuation (em dash U+2014, en dash U+2013, ellipsis U+2026, etc.)
        0x2100, 0x214F, // Letterlike Symbols
        0x2190, 0x21FF, // Arrows
        0,
    };

    // Embedded DejaVu Sans Mono as the default editor font
    ImFontConfig embedded_cfg;
    embedded_cfg.FontDataOwnedByAtlas = false; // we own the static data
    ImFont* default_editor_font = io.Fonts->AddFontFromMemoryTTF(
        (void*)DejaVuSansMono_ttf_data, DejaVuSansMono_ttf_size, 16.0f, &embedded_cfg, extended_glyph_ranges);
    editor_fonts.push_back({"default", "DejaVu Sans Mono (Built-in)", default_editor_font});
    ImFont* droid_sans_font = nullptr;
    const std::string droid_sans_path = ResolveExistingPath("vendor/recastnavigation/RecastDemo/Bin/DroidSans.ttf", executable_dir);
    if (FileExists(droid_sans_path))
    {
        droid_sans_font = io.Fonts->AddFontFromFileTTF(droid_sans_path.c_str(), 16.0f, nullptr, extended_glyph_ranges);
        if (droid_sans_font != nullptr)
        {
            editor_fonts.push_back({"droid_sans", "Vendor: Droid Sans", droid_sans_font});
        }
    }

    for (const auto& [font_path, label] : discovered_fonts)
    {
        ImFont* font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 16.0f, nullptr, extended_glyph_ranges);
        if (font != nullptr)
        {
            editor_fonts.push_back({font_path, label, font});
        }
    }
    for (const EditorFontEntry& font_entry : editor_fonts)
    {
        ImGuiFontAwesome::MergeSolid(io, font_entry.font, 13.0f);
    }

    if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context))
    {
        std::fprintf(stderr, "ImGui_ImplSDL2_InitForOpenGL failed\n");
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        std::fprintf(stderr, "ImGui_ImplOpenGL3_Init failed\n");
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    EditorSettings settings;
    LoadSettings(settings);
    if (settings.bugl_path.empty())
    {
        settings.bugl_path = GetDefaultBuglPath();
    }
    if (settings.bytecode_output_path.empty())
    {
        settings.bytecode_output_path = GetDefaultBytecodePath(settings.last_file_path);
    }

    auto find_font_entry = [&](const std::string& id) -> const EditorFontEntry*
    {
        auto it = std::find_if(editor_fonts.begin(), editor_fonts.end(), [&](const EditorFontEntry& entry)
        {
            return entry.id == id;
        });
        return it != editor_fonts.end() ? &(*it) : nullptr;
    };

    if (settings.font_choice == EditorFontChoice::DroidSans && find_font_entry("droid_sans") == nullptr)
    {
        // Try to auto-select a system monospace font (DejaVu Sans Mono preferred)
        const EditorFontEntry* mono_font = nullptr;
        for (const auto& entry : editor_fonts)
        {
            if (entry.label.find("DejaVu") != std::string::npos ||
                entry.label.find("Liberation") != std::string::npos ||
                entry.label.find("Ubuntu") != std::string::npos ||
                entry.label.find("JetBrains") != std::string::npos ||
                entry.label.find("Fira") != std::string::npos ||
                entry.label.find("Noto") != std::string::npos)
            {
                mono_font = &entry;
                break;
            }
        }
        if (mono_font)
        {
            settings.font_choice = EditorFontChoice::Custom;
            settings.font_path = mono_font->id;
        }
        else
        {
            settings.font_choice = EditorFontChoice::Default;
            settings.font_path.clear();
        }
    }
    if (settings.font_choice == EditorFontChoice::Custom && find_font_entry(settings.font_path) == nullptr)
    {
        settings.font_choice = EditorFontChoice::Default;
        settings.font_path.clear();
    }
    SaveSettings(settings);

    // ── Tab system ──
    std::vector<std::unique_ptr<EditorTab>> tabs;
    int active_tab_index = 0;
    int switch_to_tab = -1;
    EditorTab* at = nullptr;

    auto init_tab_editor = [&](TextEditor& ed)
    {
        ed.SetPalette(settings.palette);
        ed.SetLanguageDefinition(TextEditor::LanguageDefinitionId::BuLang);
        ed.SetShowWhitespacesEnabled(false);
        ed.SetShortTabsEnabled(true);
        ed.SetTabSize(4);
        ed.SetAutoIndentEnabled(true);
        ed.SetFontScale(settings.font_scale);
    };

    auto create_tab = [&](const std::string& path = "") -> int
    {
        auto tab = std::make_unique<EditorTab>();
        init_tab_editor(tab->editor);
        tab->file_path = path;
        tab->last_edit_ticks = SDL_GetTicks();
        tabs.push_back(std::move(tab));
        return static_cast<int>(tabs.size()) - 1;
    };

    auto find_tab_by_path = [&](const std::string& path) -> int
    {
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
        {
            if (tabs[i]->file_path == path) return i;
        }
        return -1;
    };

    create_tab(ResolveExistingPath(settings.last_file_path, executable_dir));
    active_tab_index = 0;
    at = tabs[0].get();

    bool done = false;
    std::string bugl_path = ResolveExistingPath(settings.bugl_path, executable_dir);
    std::string bytecode_output_path = settings.bytecode_output_path.empty()
        ? ResolveWritablePath(GetDefaultBytecodePath(at->file_path), executable_dir)
        : ResolveWritablePath(settings.bytecode_output_path, executable_dir);
    std::string status = "Ready";
    std::string command_output;
    std::string last_command_label;
    std::string last_command_line;
    EditorFontChoice font_choice = settings.font_choice;
    ImGuiFileDialog file_dialog;
    GifRecorder gif_recorder;
    ImGuiConsole console;
    FindReplaceState find_replace;
    GoToLineState go_to_line;
    FontPickerState font_picker;
    SnippetPopupState snippet_popup;
    UnsavedCloseState unsaved_close;
    OutlineState outline;
    outline.visible = settings.outline_visible;
    outline.side = settings.outline_side;
    FileExplorerState file_explorer;
    file_explorer.visible = settings.file_explorer_visible;
    file_explorer.root.name = project_dir.filename().string();
    file_explorer.root.full_path = project_dir.string();
    file_explorer.root.is_directory = true;
    MinimapState minimap;
    minimap.visible = settings.minimap_visible;
    console.SetVisible(settings.console_visible);
    auto command_state = std::make_shared<AsyncCommandState>();

    auto apply_palette = [&](TextEditor::PaletteId palette, const char* label)
    {
        settings.palette = palette;
        for (auto& tab : tabs) tab->editor.SetPalette(palette);
        SaveSettings(settings);
        status = std::string("Theme: ") + label;
    };
    auto selected_font = [&]() -> ImFont*
    {
        if (font_choice == EditorFontChoice::DroidSans)
        {
            if (const EditorFontEntry* entry = find_font_entry("droid_sans"))
            {
                return entry->font;
            }
        }
        if (font_choice == EditorFontChoice::Custom)
        {
            if (const EditorFontEntry* entry = find_font_entry(settings.font_path))
            {
                return entry->font;
            }
        }
        return default_editor_font;
    };
    auto selected_font_name = [&]() -> std::string
    {
        if (font_choice == EditorFontChoice::DroidSans)
        {
            if (const EditorFontEntry* entry = find_font_entry("droid_sans"))
            {
                return entry->label;
            }
        }
        if (font_choice == EditorFontChoice::Custom)
        {
            if (const EditorFontEntry* entry = find_font_entry(settings.font_path))
            {
                return entry->label;
            }
        }
        return "Default";
    };
    auto select_font = [&](EditorFontChoice choice, const std::string& font_id, const char* label)
    {
        font_choice = choice;
        settings.font_choice = choice;
        settings.font_path = font_id;
        SaveSettings(settings);
        status = std::string("Font: ") + label;
    };
    auto open_font_picker = [&]()
    {
        font_picker.visible = true;
        font_picker.focus_filter = true;
        font_picker.filter.clear();
    };
    auto adjust_zoom = [&](float delta)
    {
        const float new_scale = at->editor.GetFontScale() + delta;
        for (auto& tab : tabs) tab->editor.SetFontScale(new_scale);
        settings.font_scale = new_scale;
        SaveSettings(settings);
        status = "Zoom: " + std::to_string(static_cast<int>(new_scale * 100.0f)) + "%";
    };
    auto sync_paths_to_settings = [&]()
    {
        settings.last_file_path = at->file_path;
        settings.bugl_path = bugl_path;
        settings.bytecode_output_path = bytecode_output_path;
        SaveSettings(settings);
    };
    auto toggle_gif_recording = [&]()
    {
        int drawable_w = 0;
        int drawable_h = 0;
        SDL_GL_GetDrawableSize(window, &drawable_w, &drawable_h);
        gif_recorder.Toggle(project_dir, drawable_w, drawable_h, status);
    };
    auto launch_command = [&](const std::string& label, const std::string& command)
    {
        {
            std::lock_guard<std::mutex> lock(command_state->mutex);
            if (command_state->running)
            {
                status = "Another BuGL task is already running.";
                return false;
            }

            command_state->running = true;
            command_state->has_result = false;
            command_state->exit_code = -1;
            command_state->label = label;
            command_state->command = command;
            command_state->output.clear();
        }

        last_command_label = label;
        last_command_line = command;
        command_output = "$ " + command + "\n";
        status = label + " started";
        console.SetVisible(true);
        settings.console_visible = console.IsVisible();
        SaveSettings(settings);

        auto shared_state = command_state;
        std::thread([shared_state, label, command, project_dir]()
        {
            std::string output;
            int exit_code = -1;
            const std::string shell_command = BuildShellCommand(project_dir, command) + " 2>&1";
            FILE* pipe = OpenPipe(shell_command.c_str(), "r");
            if (!pipe)
            {
                output = "Failed to start process.\n";
            }
            else
            {
                char buffer[512];
                while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr)
                {
                    output += buffer;
                    std::lock_guard<std::mutex> lock(shared_state->mutex);
                    shared_state->output = output;
                }
                exit_code = ClosePipe(pipe);
            }

            std::lock_guard<std::mutex> lock(shared_state->mutex);
            shared_state->running = false;
            shared_state->has_result = true;
            shared_state->exit_code = exit_code;
            shared_state->label = label;
            shared_state->command = command;
            shared_state->output = output;
        }).detach();

        return true;
    };
    auto navigate_to_line = [&](int line)
    {
        const int clamped_line = std::max(0, std::min(line, at->editor.GetLineCount() - 1));
        at->editor.SetCursorPosition(clamped_line, 0);
        at->editor.SetViewAtLine(clamped_line, TextEditor::SetViewAtLineMode::Centered);
        status = "Line " + std::to_string(clamped_line + 1);
    };
    auto find_next = [&](bool forward)
    {
        const std::string current_text = at->editor.GetText();
        if (find_replace.find_text.empty() || current_text.empty())
        {
            status = "Find text is empty";
            return false;
        }

        const std::vector<size_t> line_offsets = BuildLineOffsets(current_text);
        const TextEditor::SelectionPosition selection = at->editor.GetSelectionPosition();
        const bool has_selection =
            selection.start.line >= 0 &&
            (selection.start.line != selection.end.line || selection.start.column != selection.end.column);
        const TextEditor::TextPosition cursor = at->editor.GetCursorPosition();
        const size_t start_offset = has_selection
            ? LineColumnToOffset(line_offsets,
                                 forward ? selection.end.line : selection.start.line,
                                 forward ? selection.end.column : selection.start.column)
            : LineColumnToOffset(line_offsets, cursor.line, cursor.column);

        size_t match_start = 0;
        size_t match_end = 0;
        if (!FindTextRange(current_text, find_replace.find_text, find_replace.case_sensitive, start_offset, forward, match_start, match_end))
        {
            status = "No matches for \"" + find_replace.find_text + "\"";
            return false;
        }

        const TextEditor::TextPosition start = OffsetToTextPosition(line_offsets, match_start);
        const TextEditor::TextPosition end = OffsetToTextPosition(line_offsets, match_end);
        at->editor.SetSelectionPosition({start, end});
        at->editor.SetViewAtLine(start.line, TextEditor::SetViewAtLineMode::Centered);
        status = "Match at line " + std::to_string(start.line + 1);
        return true;
    };
    auto replace_current = [&]()
    {
        if (find_replace.find_text.empty())
        {
            status = "Find text is empty";
            return false;
        }

        std::string current_text = at->editor.GetText();
        const TextEditor::SelectionPosition selection = at->editor.GetSelectionPosition();
        const std::string selected_text = at->editor.GetSelectedText();
        if (selected_text.empty() || !SelectionMatchesQuery(selected_text, find_replace.find_text, find_replace.case_sensitive))
        {
            if (!find_next(true))
            {
                return false;
            }
            current_text = at->editor.GetText();
        }

        const std::vector<size_t> line_offsets = BuildLineOffsets(current_text);
        const TextEditor::SelectionPosition updated_selection = at->editor.GetSelectionPosition();
        const size_t start_offset = LineColumnToOffset(line_offsets, updated_selection.start.line, updated_selection.start.column);
        const size_t end_offset = LineColumnToOffset(line_offsets, updated_selection.end.line, updated_selection.end.column);
        current_text.replace(start_offset, end_offset - start_offset, find_replace.replace_text);
        at->editor.SetText(current_text);

        const std::vector<size_t> new_offsets = BuildLineOffsets(current_text);
        const TextEditor::TextPosition new_start = OffsetToTextPosition(new_offsets, start_offset);
        const TextEditor::TextPosition new_end = OffsetToTextPosition(new_offsets, start_offset + find_replace.replace_text.size());
        at->editor.SetSelectionPosition({new_start, new_end});
        at->editor.SetViewAtLine(new_start.line, TextEditor::SetViewAtLineMode::Centered);
        status = "Replaced selection";
        return true;
    };
    auto replace_all = [&]()
    {
        if (find_replace.find_text.empty())
        {
            status = "Find text is empty";
            return false;
        }

        const std::string source_text = at->editor.GetText();
        if (source_text.empty())
        {
            status = "Document is empty";
            return false;
        }

        std::string haystack = find_replace.case_sensitive ? source_text : ToLowerCopy(source_text);
        const std::string token = find_replace.case_sensitive ? find_replace.find_text : ToLowerCopy(find_replace.find_text);
        std::string result;
        result.reserve(source_text.size());

        size_t cursor_offset = 0;
        size_t replaced_count = 0;
        while (cursor_offset < haystack.size())
        {
            const size_t found = haystack.find(token, cursor_offset);
            if (found == std::string::npos)
            {
                result.append(source_text.substr(cursor_offset));
                break;
            }
            result.append(source_text.substr(cursor_offset, found - cursor_offset));
            result.append(find_replace.replace_text);
            cursor_offset = found + token.size();
            ++replaced_count;
        }

        if (replaced_count == 0)
        {
            status = "No matches for \"" + find_replace.find_text + "\"";
            return false;
        }

        at->editor.SetText(result);
        status = "Replaced " + std::to_string(replaced_count) + " matches";
        return true;
    };

    auto run_new = [&]()
    {
        const int idx = create_tab();
        active_tab_index = idx;
        at = tabs[idx].get();
        switch_to_tab = idx;
        status = "New file";
    };
    auto open_open_popup = [&]()
    {
        std::filesystem::path start_directory = GetProjectDirectory(executable_dir) / "scripts";
        if (!at->file_path.empty())
        {
            start_directory = std::filesystem::path(ResolveExistingPath(at->file_path, executable_dir)).parent_path();
        }
        file_dialog.Open(ImGuiFileDialog::Mode::OpenFile,
                         start_directory,
                         std::filesystem::path(at->file_path).filename().string());
    };
    auto open_save_as_popup = [&]()
    {
        std::filesystem::path start_directory = GetProjectDirectory(executable_dir) / "scripts";
        const std::string seed_path = at->file_path.empty() ? settings.last_file_path : at->file_path;
        if (!seed_path.empty())
        {
            start_directory = std::filesystem::path(ResolveWritablePath(seed_path, executable_dir)).parent_path();
        }
        file_dialog.Open(ImGuiFileDialog::Mode::SaveFile,
                         start_directory,
                         std::filesystem::path(seed_path).filename().string());
    };
    auto run_open = [&]()
    {
        if (at->file_path.empty())
        {
            status = "Set a file path before opening.";
            return;
        }

        std::string loaded_text;
        const std::string resolved_path = ResolveExistingPath(at->file_path, executable_dir);
        if (LoadTextFile(resolved_path, loaded_text, status))
        {
            at->file_path = resolved_path;
            at->editor.SetText(loaded_text);
            at->last_saved_text = loaded_text;
            at->last_seen_text = loaded_text;
            at->outline_symbols = ScanDocumentSymbols(loaded_text);
            settings.last_file_path = at->file_path;
            bytecode_output_path = ResolveWritablePath(GetDefaultBytecodePath(at->file_path), executable_dir);
            settings.bytecode_output_path = bytecode_output_path;
            SaveSettings(settings);
        }
    };
    auto open_file_in_tab = [&](const std::string& path)
    {
        const std::string resolved = ResolveExistingPath(path, executable_dir);
        int existing = find_tab_by_path(resolved);
        if (existing >= 0)
        {
            active_tab_index = existing;
            at = tabs[existing].get();
            switch_to_tab = existing;
            status = "Switched to " + at->GetTitle();
            return;
        }
        const int idx = create_tab(resolved);
        active_tab_index = idx;
        at = tabs[idx].get();
        switch_to_tab = idx;
        run_open();
    };
    auto run_save = [&]()
    {
        if (at->file_path.empty())
        {
            status = "Set a file path before saving.";
            return;
        }

        const std::string resolved_path = ResolveWritablePath(at->file_path, executable_dir);
        const std::string current_text = at->editor.GetText();
        if (SaveTextFile(resolved_path, current_text, status))
        {
            at->file_path = resolved_path;
            at->last_saved_text = current_text;
            at->last_seen_text = current_text;
            at->last_autosave_ticks = SDL_GetTicks();
            settings.last_file_path = at->file_path;
            SaveSettings(settings);
        }
    };
    auto ensure_script_ready = [&]() -> bool
    {
        if (at->file_path.empty())
        {
            status = "Set a script path first.";
            return false;
        }
        bugl_path = ResolveExistingPath(bugl_path, executable_dir);
        if (bugl_path.empty())
        {
            status = "Set the BuGL executable path first.";
            return false;
        }
        if (!FileExists(bugl_path))
        {
            status = "BuGL executable not found: " + bugl_path;
            return false;
        }

        at->file_path = ResolveWritablePath(at->file_path, executable_dir);
        const std::string current_text = at->editor.GetText();
        if (current_text != at->last_saved_text)
        {
            run_save();
            if (at->editor.GetText() != at->last_saved_text)
            {
                return false;
            }
        }

        sync_paths_to_settings();
        return true;
    };
    auto run_script = [&]()
    {
        if (!ensure_script_ready())
        {
            return;
        }

        const std::string command =
            QuoteCommandArgument(bugl_path) + " " + QuoteCommandArgument(at->file_path);
        launch_command("Run Script", command);
    };
    auto run_compile_bytecode = [&]()
    {
        if (!ensure_script_ready())
        {
            return;
        }

        bytecode_output_path = ResolveWritablePath(
            bytecode_output_path.empty() ? GetDefaultBytecodePath(at->file_path) : bytecode_output_path,
            executable_dir);

        sync_paths_to_settings();
        const std::string command =
            QuoteCommandArgument(bugl_path) + " --compile-bc " +
            QuoteCommandArgument(at->file_path) + " " +
            QuoteCommandArgument(bytecode_output_path);
        launch_command("Compile Bytecode", command);
    };

    auto close_tab = [&](int index)
    {
        if (index < 0 || index >= static_cast<int>(tabs.size()))
        {
            return;
        }
        if (tabs[index]->IsDirty())
        {
            unsaved_close.visible = true;
            unsaved_close.tab_index = index;
            unsaved_close.closing_app = false;
            return;
        }
        tabs.erase(tabs.begin() + index);
        if (tabs.empty())
        {
            create_tab();
        }
        if (active_tab_index >= static_cast<int>(tabs.size()))
        {
            active_tab_index = static_cast<int>(tabs.size()) - 1;
        }
        at = tabs[active_tab_index].get();
        status = "Tab closed";
    };

    auto force_close_tab = [&](int index)
    {
        if (index < 0 || index >= static_cast<int>(tabs.size()))
        {
            return;
        }
        tabs.erase(tabs.begin() + index);
        if (tabs.empty())
        {
            create_tab();
        }
        if (active_tab_index >= static_cast<int>(tabs.size()))
        {
            active_tab_index = static_cast<int>(tabs.size()) - 1;
        }
        at = tabs[active_tab_index].get();
        status = "Tab closed";
    };

    auto insert_snippet = [&](const BuLangSnippet& snippet)
    {
        int cursor_offset = -1;
        const std::string text = SnippetManager::PrepareSnippetText(snippet, cursor_offset);
        // Get current text, insert at cursor position, set text
        const std::string current = at->editor.GetText();
        const auto pos = at->editor.GetCursorPosition();
        const std::vector<size_t> offsets = BuildLineOffsets(current);
        const size_t insert_offset = LineColumnToOffset(offsets, pos.line, pos.column);
        std::string updated = current;
        updated.insert(insert_offset, text);
        at->editor.SetText(updated);
        // Place cursor after inserted text
        const std::vector<size_t> new_offsets = BuildLineOffsets(updated);
        const size_t new_cursor_offset = (cursor_offset >= 0)
            ? insert_offset + static_cast<size_t>(cursor_offset)
            : insert_offset + text.size();
        const auto new_pos = OffsetToTextPosition(new_offsets, new_cursor_offset);
        at->editor.SetCursorPosition(new_pos.line, new_pos.column);
        status = "Snippet: " + snippet.name;
    };

    if (!at->file_path.empty())
    {
        run_open();
    }

    while (!done)
    {
        // Update active tab pointer
        if (active_tab_index >= 0 && active_tab_index < static_cast<int>(tabs.size()))
        {
            at = tabs[active_tab_index].get();
        }

        {
            std::lock_guard<std::mutex> lock(command_state->mutex);
            if (command_state->running)
            {
                last_command_label = command_state->label;
                last_command_line = command_state->command;
                command_output = "$ " + command_state->command + "\n" + command_state->output;
            }
            if (command_state->has_result)
            {
                last_command_label = command_state->label;
                last_command_line = command_state->command;
                command_output = "$ " + command_state->command + "\n" + command_state->output;
                status = command_state->exit_code == 0
                    ? command_state->label + " finished"
                    : command_state->label + " failed (" + std::to_string(command_state->exit_code) + ")";
                command_state->has_result = false;
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
            {
                // Check for unsaved changes before quitting
                bool has_unsaved = false;
                for (const auto& tab : tabs)
                {
                    if (tab->IsDirty()) { has_unsaved = true; break; }
                }
                if (has_unsaved)
                {
                    unsaved_close.visible = true;
                    unsaved_close.tab_index = -1;
                    unsaved_close.closing_app = true;
                }
                else
                {
                    done = true;
                }
            }
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
            {
                bool has_unsaved = false;
                for (const auto& tab : tabs)
                {
                    if (tab->IsDirty()) { has_unsaved = true; break; }
                }
                if (has_unsaved)
                {
                    unsaved_close.visible = true;
                    unsaved_close.tab_index = -1;
                    unsaved_close.closing_app = true;
                }
                else
                {
                    done = true;
                }
            }
            // Drag & Drop: open dropped files
            if (event.type == SDL_DROPFILE)
            {
                const char* dropped_file = event.drop.file;
                if (dropped_file)
                {
                    open_file_in_tab(std::string(dropped_file));
                }
                SDL_free(event.drop.file);
            }
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
            {
                const bool ctrl_down = (event.key.keysym.mod & KMOD_CTRL) != 0;
                const bool alt_down = (event.key.keysym.mod & KMOD_ALT) != 0;
                const bool ralt_down = (event.key.keysym.mod & KMOD_RALT) != 0;
                // AltGr detection: on some systems AltGr = Ctrl+Alt, on others just RALT
                const bool altgr_down = ralt_down || (ctrl_down && alt_down);

                // Normal Ctrl shortcuts (skip when AltGr is active)
                if (ctrl_down && !altgr_down)
                {
                    if (event.key.keysym.sym == SDLK_n)
                    {
                        run_new();
                    }
                    else if (event.key.keysym.sym == SDLK_o)
                    {
                        open_open_popup();
                    }
                    else if (event.key.keysym.sym == SDLK_s)
                    {
                        if ((event.key.keysym.mod & KMOD_SHIFT) != 0)
                        {
                            open_save_as_popup();
                        }
                        else
                        {
                            if (at->file_path.empty())
                            {
                                open_save_as_popup();
                            }
                            else
                            {
                                run_save();
                            }
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_f)
                    {
                        if ((event.key.keysym.mod & KMOD_SHIFT) != 0)
                        {
                            at->editor.FormatAll();
                            status = "Code formatted";
                        }
                        else
                        {
                            find_replace.visible = true;
                            find_replace.replace_visible = false;
                            find_replace.focus_find_input = true;
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_h)
                    {
                        find_replace.visible = true;
                        find_replace.replace_visible = true;
                        find_replace.focus_find_input = true;
                    }
                    else if (event.key.keysym.sym == SDLK_g)
                    {
                        go_to_line.visible = true;
                        go_to_line.focus_input = true;
                    }
                    else if (event.key.keysym.sym == SDLK_EQUALS || event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS)
                    {
                        adjust_zoom(0.1f);
                    }
                    else if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS)
                    {
                        adjust_zoom(-0.1f);
                    }
                    else if (event.key.keysym.sym == SDLK_0 || event.key.keysym.sym == SDLK_KP_0)
                    {
                        for (auto& tab : tabs) tab->editor.SetFontScale(1.0f);
                        settings.font_scale = 1.0f;
                        SaveSettings(settings);
                        status = "Zoom reset";
                    }
                    else if (event.key.keysym.sym == SDLK_w)
                    {
                        close_tab(active_tab_index);
                    }
                    else if (event.key.keysym.sym == SDLK_SPACE)
                    {
                        snippet_popup.visible = true;
                        snippet_popup.focus_filter = true;
                        snippet_popup.filter.clear();
                    }
                    else if (event.key.keysym.sym == SDLK_b)
                    {
                        file_explorer.visible = !file_explorer.visible;
                        settings.file_explorer_visible = file_explorer.visible;
                        SaveSettings(settings);
                        status = file_explorer.visible ? "Explorer shown" : "Explorer hidden";
                    }
                    else if (event.key.keysym.sym == SDLK_SLASH || event.key.keysym.sym == SDLK_KP_DIVIDE ||
                             event.key.keysym.scancode == SDL_SCANCODE_SLASH)
                    {
                        at->editor.ToggleComment();
                    }
                    else if (event.key.keysym.sym == SDLK_d && (event.key.keysym.mod & KMOD_SHIFT) != 0)
                    {
                        at->editor.DuplicateLine();
                        status = "Line duplicated";
                    }
                    else if (event.key.keysym.sym == SDLK_LEFTBRACKET && (event.key.keysym.mod & KMOD_SHIFT) != 0)
                    {
                        if (at->editor.IsFoldingEnabled())
                        {
                            at->editor.FoldAll();
                            status = "All regions folded";
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_RIGHTBRACKET && (event.key.keysym.mod & KMOD_SHIFT) != 0)
                    {
                        if (at->editor.IsFoldingEnabled())
                        {
                            at->editor.UnfoldAll();
                            status = "All regions unfolded";
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_m)
                    {
                        minimap.visible = !minimap.visible;
                        settings.minimap_visible = minimap.visible;
                        SaveSettings(settings);
                        status = minimap.visible ? "Minimap shown" : "Minimap hidden";
                    }
                }
                else if (event.key.keysym.sym == SDLK_F5)
                {
                    run_script();
                }
                else if (event.key.keysym.sym == SDLK_F7)
                {
                    run_compile_bytecode();
                }
                else if (event.key.keysym.sym == SDLK_F8)
                {
                    console.SetVisible(!console.IsVisible());
                    settings.console_visible = console.IsVisible();
                    SaveSettings(settings);
                    status = console.IsVisible() ? "Console shown" : "Console hidden";
                }
                else if (event.key.keysym.sym == SDLK_F9)
                {
                    toggle_gif_recording();
                }
                else if (event.key.keysym.sym == SDLK_F3)
                {
                    find_next((event.key.keysym.mod & KMOD_SHIFT) == 0);
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                    run_new();
                }
                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                {
                    open_open_popup();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    if (at->file_path.empty())
                    {
                        open_save_as_popup();
                    }
                    else
                    {
                        run_save();
                    }
                }
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                {
                    open_save_as_popup();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Close Tab", "Ctrl+W"))
                {
                    close_tab(active_tab_index);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Autosave", nullptr, settings.autosave_enabled))
                {
                    settings.autosave_enabled = !settings.autosave_enabled;
                    SaveSettings(settings);
                    status = settings.autosave_enabled ? "Autosave enabled" : "Autosave disabled";
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    bool has_unsaved = false;
                    for (const auto& tab : tabs)
                    {
                        if (tab->IsDirty()) { has_unsaved = true; break; }
                    }
                    if (has_unsaved)
                    {
                        unsaved_close.visible = true;
                        unsaved_close.tab_index = -1;
                        unsaved_close.closing_app = true;
                    }
                    else
                    {
                        done = true;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Find", "Ctrl+F"))
                {
                    find_replace.visible = true;
                    find_replace.replace_visible = false;
                    find_replace.focus_find_input = true;
                }
                if (ImGui::MenuItem("Replace", "Ctrl+H"))
                {
                    find_replace.visible = true;
                    find_replace.replace_visible = true;
                    find_replace.focus_find_input = true;
                }
                if (ImGui::MenuItem("Find Next", "F3"))
                {
                    find_next(true);
                }
                if (ImGui::MenuItem("Find Previous", "Shift+F3"))
                {
                    find_next(false);
                }
                if (ImGui::MenuItem("Go To Line", "Ctrl+G"))
                {
                    go_to_line.visible = true;
                    go_to_line.focus_input = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicate Line", "Ctrl+Shift+D"))
                {
                    at->editor.DuplicateLine();
                    status = "Line duplicated";
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Format"))
            {
                if (ImGui::MenuItem("Format All", "Ctrl+Shift+F"))
                {
                    at->editor.FormatAll();
                    status = "Code formatted";
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Braces to New Line"))
                {
                    at->editor.FormatBracesNewLine();
                    status = "Braces moved to new lines";
                }
                if (ImGui::MenuItem("Fix Indentation"))
                {
                    at->editor.FormatIndentation();
                    status = "Indentation fixed";
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Snippets"))
            {
                for (const auto& snippet : SnippetManager::GetSnippets())
                {
                    const std::string label = snippet.name + " - " + snippet.description;
                    if (ImGui::MenuItem(label.c_str()))
                    {
                        insert_snippet(snippet);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Snippet Popup...", "Ctrl+Space"))
                {
                    snippet_popup.visible = true;
                    snippet_popup.focus_filter = true;
                    snippet_popup.filter.clear();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Build"))
            {
                if (ImGui::MenuItem("Run Script", "F5"))
                {
                    run_script();
                }
                if (ImGui::MenuItem("Compile Bytecode", "F7"))
                {
                    run_compile_bytecode();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Console", "F8", console.IsVisible()))
                {
                    console.SetVisible(!console.IsVisible());
                    settings.console_visible = console.IsVisible();
                    SaveSettings(settings);
                    status = console.IsVisible() ? "Console shown" : "Console hidden";
                }
                if (ImGui::MenuItem("Clear Output"))
                {
                    command_output.clear();
                    console.Clear();
                    std::lock_guard<std::mutex> lock(command_state->mutex);
                    command_state->output.clear();
                    last_command_label.clear();
                    last_command_line.clear();
                    status = "Output cleared";
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Zoom In", "Ctrl++"))
                {
                    adjust_zoom(0.1f);
                }
                if (ImGui::MenuItem("Zoom Out", "Ctrl+-"))
                {
                    adjust_zoom(-0.1f);
                }
                if (ImGui::MenuItem("Reset Zoom", "Ctrl+0"))
                {
                    for (auto& tab : tabs) tab->editor.SetFontScale(1.0f);
                    settings.font_scale = 1.0f;
                    SaveSettings(settings);
                    status = "Zoom reset";
                }
                ImGui::Separator();
                if (ImGui::MenuItem("File Explorer", "Ctrl+B", file_explorer.visible))
                {
                    file_explorer.visible = !file_explorer.visible;
                    settings.file_explorer_visible = file_explorer.visible;
                    SaveSettings(settings);
                }
                if (ImGui::MenuItem("Minimap", "Ctrl+M", minimap.visible))
                {
                    minimap.visible = !minimap.visible;
                    settings.minimap_visible = minimap.visible;
                    SaveSettings(settings);
                }
                if (ImGui::MenuItem("Console", "F8", console.IsVisible()))
                {
                    console.SetVisible(!console.IsVisible());
                    settings.console_visible = console.IsVisible();
                    SaveSettings(settings);
                    status = console.IsVisible() ? "Console shown" : "Console hidden";
                }
                if (ImGui::MenuItem(gif_recorder.IsRecording() ? "Stop GIF Recording" : "Record GIF", "F9"))
                {
                    toggle_gif_recording();
                }
                ImGui::Separator();
                // Code folding
                if (ImGui::MenuItem("Code Folding", nullptr, at->editor.IsFoldingEnabled()))
                {
                    at->editor.SetFoldingEnabled(!at->editor.IsFoldingEnabled());
                }
                if (at->editor.IsFoldingEnabled())
                {
                    if (ImGui::MenuItem("Fold All", "Ctrl+Shift+["))
                    {
                        at->editor.FoldAll();
                        status = "All regions folded";
                    }
                    if (ImGui::MenuItem("Unfold All", "Ctrl+Shift+]"))
                    {
                        at->editor.UnfoldAll();
                        status = "All regions unfolded";
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Outline", nullptr, outline.visible))
                {
                    outline.visible = !outline.visible;
                    settings.outline_visible = outline.visible;
                    SaveSettings(settings);
                }
                if (outline.visible)
                {
                    if (ImGui::MenuItem("Outline Left", nullptr, outline.side == OutlineSide::Left))
                    {
                        outline.side = OutlineSide::Left;
                        settings.outline_side = outline.side;
                        SaveSettings(settings);
                    }
                    if (ImGui::MenuItem("Outline Right", nullptr, outline.side == OutlineSide::Right))
                    {
                        outline.side = OutlineSide::Right;
                        settings.outline_side = outline.side;
                        SaveSettings(settings);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("VSCode Dark", nullptr, at->editor.GetPalette() == TextEditor::PaletteId::VsCodeDark))
                {
                    apply_palette(TextEditor::PaletteId::VsCodeDark, "VSCode Dark");
                }
                if (ImGui::MenuItem("Mariana", nullptr, at->editor.GetPalette() == TextEditor::PaletteId::Mariana))
                {
                    apply_palette(TextEditor::PaletteId::Mariana, "Mariana");
                }
                if (ImGui::MenuItem("Dark", nullptr, at->editor.GetPalette() == TextEditor::PaletteId::Dark))
                {
                    apply_palette(TextEditor::PaletteId::Dark, "Dark");
                }
                if (ImGui::MenuItem("Light", nullptr, at->editor.GetPalette() == TextEditor::PaletteId::Light))
                {
                    apply_palette(TextEditor::PaletteId::Light, "Light");
                }
                if (ImGui::MenuItem("Retro Blue", nullptr, at->editor.GetPalette() == TextEditor::PaletteId::RetroBlue))
                {
                    apply_palette(TextEditor::PaletteId::RetroBlue, "Retro Blue");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Choose Font..."))
                {
                    open_font_picker();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::MenuItem("About BuEditor", nullptr, false, false);
                ImGui::Separator();
                ImGui::Text("Version: %s", kBuEditorVersion);
                ImGui::Text("Build: %s", kBuEditorBuildDate);
                ImGui::Text("ImGui: %s", IMGUI_VERSION);
                ImGui::Text("SDL: %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("BuEditor", nullptr,
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize);

        const std::filesystem::path scripts_dir = project_dir / "scripts";
        file_dialog.Render(project_dir, scripts_dir, executable_dir);
        if (file_dialog.HasResult())
        {
            const ImGuiFileDialog::Result result = file_dialog.ConsumeResult();
            if (result.accepted)
            {
                if (result.mode == ImGuiFileDialog::Mode::OpenFile)
                {
                    open_file_in_tab(result.path.string());
                }
                else if (result.mode == ImGuiFileDialog::Mode::SaveFile)
                {
                    at->file_path = result.path.string();
                    run_save();
                }
            }
        }

        if (find_replace.visible)
        {
            ImGui::BeginChild("##find_panel", ImVec2(-1.0f, find_replace.replace_visible ? 82.0f : 48.0f), true);
            ImGui::SetNextItemWidth(260.0f);
            if (ImGui::InputTextWithHint("##find_text", "Find", &find_replace.find_text, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                find_next(true);
            }
            if (find_replace.focus_find_input)
            {
                ImGui::SetKeyboardFocusHere(-1);
                find_replace.focus_find_input = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Next"))
            {
                find_next(true);
            }
            ImGui::SameLine();
            if (ImGui::Button("Prev"))
            {
                find_next(false);
            }
            ImGui::SameLine();
            ImGui::Checkbox("Case", &find_replace.case_sensitive);
            ImGui::SameLine();
            if (ImGui::Button(find_replace.replace_visible ? "Hide Replace" : "Replace"))
            {
                find_replace.replace_visible = !find_replace.replace_visible;
                find_replace.focus_replace_input = find_replace.replace_visible;
            }
            ImGui::SameLine();
            if (ImGui::Button("Close"))
            {
                find_replace.visible = false;
                find_replace.replace_visible = false;
            }

            if (find_replace.replace_visible)
            {
                ImGui::SetNextItemWidth(260.0f);
                if (ImGui::InputTextWithHint("##replace_text", "Replace", &find_replace.replace_text, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    replace_current();
                }
                if (find_replace.focus_replace_input)
                {
                    ImGui::SetKeyboardFocusHere(-1);
                    find_replace.focus_replace_input = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Replace One"))
                {
                    replace_current();
                }
                ImGui::SameLine();
                if (ImGui::Button("Replace All"))
                {
                    replace_all();
                }
            }
            ImGui::EndChild();
        }

        if (go_to_line.visible)
        {
            ImGui::BeginChild("##goto_panel", ImVec2(-1.0f, 48.0f), true);
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::InputTextWithHint("##goto_line", "Line number", &go_to_line.line_text, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                navigate_to_line(std::max(1, std::atoi(go_to_line.line_text.c_str())) - 1);
            }
            if (go_to_line.focus_input)
            {
                ImGui::SetKeyboardFocusHere(-1);
                go_to_line.focus_input = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Go"))
            {
                navigate_to_line(std::max(1, std::atoi(go_to_line.line_text.c_str())) - 1);
            }
            ImGui::SameLine();
            if (ImGui::Button("Close##goto"))
            {
                go_to_line.visible = false;
            }
            ImGui::EndChild();
        }

        if (font_picker.visible)
        {
            ImGui::OpenPopup("Choose Font");
            font_picker.visible = false;
        }
        if (ImGui::BeginPopupModal("Choose Font", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (font_picker.focus_filter)
            {
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::SetNextItemWidth(360.0f);
            ImGui::InputTextWithHint("##font_filter", "Filter fonts", &font_picker.filter);
            if (font_picker.focus_filter)
            {
                font_picker.focus_filter = false;
            }

            const std::string filter = ToLowerCopy(font_picker.filter);
            auto matches_filter = [&](const std::string& value) -> bool
            {
                return filter.empty() || ToLowerCopy(value).find(filter) != std::string::npos;
            };
            auto render_font_option = [&](const char* label, EditorFontChoice choice, const std::string& font_id)
            {
                const bool selected =
                    (choice == EditorFontChoice::Default && font_choice == EditorFontChoice::Default) ||
                    (choice == EditorFontChoice::DroidSans && font_choice == EditorFontChoice::DroidSans) ||
                    (choice == EditorFontChoice::Custom && font_choice == EditorFontChoice::Custom && settings.font_path == font_id);
                if (!matches_filter(label))
                {
                    return;
                }
                if (ImGui::Selectable(label, selected))
                {
                    select_font(choice, font_id, label);
                    ImGui::CloseCurrentPopup();
                }
            };

            ImGui::BeginChild("##font_list", ImVec2(480.0f, 320.0f), true);
            render_font_option("Default", EditorFontChoice::Default, "");
            if (find_font_entry("droid_sans") != nullptr)
            {
                render_font_option("Vendor: Droid Sans", EditorFontChoice::DroidSans, "");
            }
            for (const EditorFontEntry& font_entry : editor_fonts)
            {
                if (font_entry.id == "default" || font_entry.id == "droid_sans")
                {
                    continue;
                }
                render_font_option(font_entry.label.c_str(), EditorFontChoice::Custom, font_entry.id);
            }
            ImGui::EndChild();

            if (ImGui::Button("Close"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        const bool command_running = [&]()
        {
            std::lock_guard<std::mutex> lock(command_state->mutex);
            return command_state->running;
        }();
        const std::string current_text = at->editor.GetText();
        if (current_text != at->last_seen_text)
        {
            at->outline_symbols = ScanDocumentSymbols(current_text);
        }
        if (command_running)
        {
            console.SetVisible(true);
        }
        console.SetText(command_output);
        settings.console_visible = console.IsVisible();
        const bool show_output_panel = console.IsVisible();

        const float output_panel_height = show_output_panel ? 160.0f : 0.0f;
        const float footer_height = show_output_panel
            ? output_panel_height + (ImGui::GetFrameHeightWithSpacing() * 5.0f)
            : ImGui::GetFrameHeightWithSpacing() * 2.0f;
        const bool parent_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        const float full_region_height = std::max(120.0f, ImGui::GetContentRegionAvail().y - footer_height);

        // ── File Explorer (left side, full height including tabs) ──
        if (file_explorer.visible)
        {
            file_explorer.width = std::clamp(file_explorer.width, 140.0f, std::max(180.0f, ImGui::GetContentRegionAvail().x - 400.0f));
            ImGuiFileExplorer::Render("##file_explorer", file_explorer.root,
                                      file_explorer.width, full_region_height,
                                      at->file_path,
                                      [&](const std::string& path) { open_file_in_tab(path); });
            ImGui::SameLine(0.0f, 0.0f);
            ImGuiSplitter::Vertical("explorer_splitter", &file_explorer.width, 140.0f, 260.0f, full_region_height);
            ImGui::SameLine(0.0f, 0.0f);
        }

        // ── Right side group: tabs + editor + outline ──
        ImGui::BeginGroup();

        // ── Tab bar ──
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll))
        {
            for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
            {
                ImGuiTabItemFlags tab_flags = 0;
                if (switch_to_tab == i)
                {
                    tab_flags |= ImGuiTabItemFlags_SetSelected;
                }
                bool tab_open = true;
                const std::string tab_label = tabs[i]->GetDisplayTitle() + "###tab" + std::to_string(i);
                if (ImGui::BeginTabItem(tab_label.c_str(), &tab_open, tab_flags))
                {
                    if (active_tab_index != i)
                    {
                        active_tab_index = i;
                        at = tabs[i].get();
                    }
                    ImGui::EndTabItem();
                }
                if (!tab_open)
                {
                    close_tab(i);
                    if (i <= active_tab_index && active_tab_index > 0)
                    {
                        --active_tab_index;
                    }
                    at = tabs.empty() ? nullptr : tabs[active_tab_index].get();
                    --i;
                }
            }
            switch_to_tab = -1;
            ImGui::EndTabBar();
        }

        const float editor_region_height = std::max(80.0f, ImGui::GetContentRegionAvail().y - footer_height);
        int cursor_line = 0;
        int cursor_column = 0;
        at->editor.GetCursorPosition(cursor_line, cursor_column);
        const auto select_symbol_block = [&](const DocumentSymbol& symbol)
        {
            TextEditor::SelectionPosition selection;
            selection.start.line = symbol.line;
            selection.start.column = 0;
            selection.end.line = symbol.end_line;
            selection.end.column = std::numeric_limits<int>::max();
            at->editor.SetSelectionPosition(selection);
            at->editor.SetViewAtLine(symbol.line, TextEditor::SetViewAtLineMode::Centered);
            status = "Selected " + symbol.name;
        };
        const DocumentSymbol* active_symbol = FindInnermostSymbolAtLine(at->outline_symbols, cursor_line);

        const auto render_outline_panel = [&]()
        {
            ImGui::BeginChild("##outline_panel", ImVec2(outline.width, editor_region_height), true);
            ImGui::TextUnformatted("Outline");
            ImGui::Separator();

            auto render_symbols = [&](auto&& self, const std::deque<DocumentSymbol>& symbols) -> void
            {
                for (const DocumentSymbol& symbol : symbols)
                {
                    const std::string label = std::string(SymbolIcon(symbol.kind)) + " " + symbol.name;
                    const bool selected_symbol = active_symbol == &symbol;
                    if (symbol.children.empty())
                    {
                        if (ImGui::Selectable(label.c_str(), selected_symbol))
                        {
                            navigate_to_line(symbol.line);
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            select_symbol_block(symbol);
                        }
                    }
                    else
                    {
                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
                        if (selected_symbol)
                        {
                            flags |= ImGuiTreeNodeFlags_Selected;
                        }
                        const bool open = ImGui::TreeNodeEx((label + "##" + std::to_string(symbol.line)).c_str(), flags);
                        if (ImGui::IsItemClicked())
                        {
                            navigate_to_line(symbol.line);
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            select_symbol_block(symbol);
                        }
                        if (open)
                        {
                            self(self, symbol.children);
                            ImGui::TreePop();
                        }
                    }
                }
            };
            render_symbols(render_symbols, at->outline_symbols);
            ImGui::EndChild();
        };

        if (outline.visible)
        {
            outline.width = std::clamp(outline.width, 160.0f, std::max(200.0f, ImGui::GetContentRegionAvail().x - 240.0f));
        }

        if (outline.visible && outline.side == OutlineSide::Left)
        {
            render_outline_panel();
            ImGui::SameLine(0.0f, 0.0f);
            ImGuiSplitter::Vertical("outline_splitter", &outline.width, 160.0f, 260.0f, editor_region_height);
            ImGui::SameLine(0.0f, 0.0f);
        }

        float editor_width = -1.0f;
        {
            float taken = 0.0f;
            if (outline.visible && outline.side == OutlineSide::Right)
            {
                taken += outline.width + 6.0f;
            }
            if (minimap.visible)
            {
                taken += minimap.width + 6.0f;
            }
            if (taken > 0.0f)
            {
                editor_width = std::max(80.0f, ImGui::GetContentRegionAvail().x - taken);
            }
        }

        ImGui::PushFont(selected_font());
        at->editor.Render("##source", parent_focused, ImVec2(editor_width, editor_region_height), false);

        // ── Right-click context menu ──
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup("##editor_context");

        if (ImGui::BeginPopup("##editor_context"))
        {
            if (ImGui::MenuItem("Cut", "Ctrl+X", false, at->editor.AnyCursorHasSelection()))
            {
                at->editor.Cut();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, at->editor.AnyCursorHasSelection()))
            {
                at->editor.Copy();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V"))
            {
                at->editor.Paste();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A"))
            {
                at->editor.SelectAll();
            }
            if (ImGui::MenuItem("Select Word", "Ctrl+D"))
            {
                at->editor.AddNextOccurrence();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, at->editor.CanUndo()))
            {
                at->editor.Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, at->editor.CanRedo()))
            {
                at->editor.Redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Find...", "Ctrl+F"))
            {
                find_replace.visible = true;
                find_replace.replace_visible = false;
                find_replace.focus_find_input = true;
            }
            if (ImGui::MenuItem("Replace...", "Ctrl+H"))
            {
                find_replace.visible = true;
                find_replace.replace_visible = true;
                find_replace.focus_find_input = true;
            }
            if (ImGui::MenuItem("Go To Line...", "Ctrl+G"))
            {
                go_to_line.visible = true;
                go_to_line.focus_input = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Toggle Comment", "Ctrl+/"))
            {
                at->editor.ToggleComment();
            }
            if (ImGui::MenuItem("Indent", "Ctrl+]"))
            {
                at->editor.Indent();
            }
            if (ImGui::MenuItem("Unindent", "Ctrl+["))
            {
                at->editor.Unindent();
            }
            if (ImGui::MenuItem("Duplicate Line", "Ctrl+Shift+D"))
            {
                at->editor.DuplicateLine();
                status = "Line duplicated";
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Format"))
            {
                if (ImGui::MenuItem("Format All", "Ctrl+Shift+F"))
                {
                    at->editor.FormatAll();
                    status = "Code formatted";
                }
                if (ImGui::MenuItem("Braces to New Line"))
                {
                    at->editor.FormatBracesNewLine();
                    status = "Braces moved to new lines";
                }
                if (ImGui::MenuItem("Fix Indentation"))
                {
                    at->editor.FormatIndentation();
                    status = "Indentation fixed";
                }
                ImGui::EndMenu();
            }
            if (at->editor.IsFoldingEnabled())
            {
                if (ImGui::BeginMenu("Folding"))
                {
                    if (ImGui::MenuItem("Fold All", "Ctrl+Shift+["))
                    {
                        at->editor.FoldAll();
                        status = "All regions folded";
                    }
                    if (ImGui::MenuItem("Unfold All", "Ctrl+Shift+]"))
                    {
                        at->editor.UnfoldAll();
                        status = "All regions unfolded";
                    }
                    ImGui::EndMenu();
                }
            }
            if (ImGui::BeginMenu("Snippets"))
            {
                for (const auto& snippet : SnippetManager::GetSnippets())
                {
                    const std::string label = snippet.name + " - " + snippet.description;
                    if (ImGui::MenuItem(label.c_str()))
                    {
                        insert_snippet(snippet);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Run Script", "F5"))
            {
                run_script();
            }
            ImGui::EndPopup();
        }

        ImGui::PopFont();

        // ── Minimap ──
        if (minimap.visible)
        {
            ImGui::SameLine(0.0f, 0.0f);
            ImGuiMinimap::Render("##minimap", at->editor, minimap.width, editor_region_height, selected_font());
        }

        if (outline.visible && outline.side == OutlineSide::Right)
        {
            ImGui::SameLine(0.0f, 0.0f);
            ImGuiSplitter::VerticalRight("outline_splitter_right", &outline.width, 260.0f, 160.0f, editor_region_height);
            ImGui::SameLine(0.0f, 0.0f);
            render_outline_panel();
        }

        ImGui::EndGroup(); // end right-side group (tabs + editor + outline)

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && io.KeyCtrl && io.MouseWheel != 0.0f)
        {
            adjust_zoom(io.MouseWheel > 0.0f ? 0.1f : -0.1f);
        }
        const bool dirty = at->IsDirty();
        if (current_text != at->last_seen_text)
        {
            at->last_edit_ticks = SDL_GetTicks();
            at->last_seen_text = current_text;
        }

        // Autosave for all dirty tabs
        if (settings.autosave_enabled)
        {
            const Uint32 now = SDL_GetTicks();
            for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
            {
                auto& tab = *tabs[i];
                if (!tab.IsDirty() || tab.file_path.empty()) continue;
                if (now - tab.last_autosave_ticks >= static_cast<Uint32>(settings.autosave_interval_ms) &&
                    now - tab.last_edit_ticks >= static_cast<Uint32>(settings.autosave_interval_ms))
                {
                    // Save this tab
                    const std::string resolved_path = ResolveWritablePath(tab.file_path, executable_dir);
                    const std::string tab_text = tab.editor.GetText();
                    std::string save_status;
                    if (SaveTextFile(resolved_path, tab_text, save_status))
                    {
                        tab.file_path = resolved_path;
                        tab.last_saved_text = tab_text;
                        tab.last_seen_text = tab_text;
                        tab.last_autosave_ticks = now;
                    }
                    if (i == active_tab_index) status = "Autosaved";
                }
            }
        }
        const char* theme_name =
            at->editor.GetPalette() == TextEditor::PaletteId::VsCodeDark ? "VSCode Dark" :
            at->editor.GetPalette() == TextEditor::PaletteId::Mariana ? "Mariana" :
            at->editor.GetPalette() == TextEditor::PaletteId::Light ? "Light" :
            at->editor.GetPalette() == TextEditor::PaletteId::RetroBlue ? "Retro Blue" : "Dark";
        const std::string font_name = selected_font_name();
        const char* gif_state = gif_recorder.IsRecording() ? "REC" : "Off";

        if (show_output_panel)
        {
            const ImGuiConsole::RenderResult console_result =
                console.Render("Console",
                               last_command_label.empty() ? nullptr : last_command_label.c_str(),
                               command_running,
                               output_panel_height);
            if (console_result.hidden)
            {
                settings.console_visible = false;
                SaveSettings(settings);
                status = "Console hidden";
            }
            if (console_result.cleared)
            {
                command_output.clear();
                std::lock_guard<std::mutex> lock(command_state->mutex);
                command_state->output.clear();
                last_command_label.clear();
                last_command_line.clear();
                status = "Output cleared";
            }
            if (console_result.copied)
            {
                status = "Console copied";
            }
        }

        // ── Snippet popup ──
        if (snippet_popup.visible)
        {
            ImGui::OpenPopup("Snippets");
            snippet_popup.visible = false;
        }
        if (ImGui::BeginPopupModal("Snippets", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (snippet_popup.focus_filter)
            {
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::SetNextItemWidth(300.0f);
            ImGui::InputTextWithHint("##snippet_filter", "Filter snippets...", &snippet_popup.filter);
            if (snippet_popup.focus_filter)
            {
                snippet_popup.focus_filter = false;
            }

            const std::string sfilter = ToLowerCopy(snippet_popup.filter);
            ImGui::BeginChild("##snippet_list", ImVec2(400.0f, 280.0f), true);
            for (const auto& snippet : SnippetManager::GetSnippets())
            {
                if (!sfilter.empty() &&
                    ToLowerCopy(snippet.name).find(sfilter) == std::string::npos &&
                    ToLowerCopy(snippet.description).find(sfilter) == std::string::npos)
                {
                    continue;
                }
                const std::string label = snippet.name + " - " + snippet.description;
                if (ImGui::Selectable(label.c_str()))
                {
                    insert_snippet(snippet);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChild();

            if (ImGui::Button("Close##snippets"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ── Unsaved changes dialog ──
        if (unsaved_close.visible)
        {
            ImGui::OpenPopup("Unsaved Changes");
            unsaved_close.visible = false;
        }
        if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const std::string msg = unsaved_close.closing_app
                ? "There are unsaved changes. Discard all and quit?"
                : "This tab has unsaved changes. Discard and close?";
            ImGui::TextUnformatted(msg.c_str());
            ImGui::Spacing();

            if (ImGui::Button("Save & Close", ImVec2(120.0f, 0.0f)))
            {
                if (unsaved_close.closing_app)
                {
                    for (auto& tab : tabs)
                    {
                        if (tab->IsDirty() && !tab->file_path.empty())
                        {
                            const std::string rp = ResolveWritablePath(tab->file_path, executable_dir);
                            std::string ss;
                            SaveTextFile(rp, tab->editor.GetText(), ss);
                        }
                    }
                    done = true;
                }
                else if (unsaved_close.tab_index >= 0 && unsaved_close.tab_index < static_cast<int>(tabs.size()))
                {
                    auto& tab = *tabs[unsaved_close.tab_index];
                    if (!tab.file_path.empty())
                    {
                        // Temporarily switch context to save
                        int old_active = active_tab_index;
                        active_tab_index = unsaved_close.tab_index;
                        at = tabs[active_tab_index].get();
                        run_save();
                        active_tab_index = old_active;
                        at = tabs[std::min(active_tab_index, static_cast<int>(tabs.size()) - 1)].get();
                    }
                    force_close_tab(unsaved_close.tab_index);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard", ImVec2(120.0f, 0.0f)))
            {
                if (unsaved_close.closing_app)
                {
                    done = true;
                }
                else
                {
                    force_close_tab(unsaved_close.tab_index);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        // Tab count indicator
        const int tab_count = static_cast<int>(tabs.size());
        std::string breadcrumb = BuildBreadcrumbPath(at->outline_symbols, cursor_line);
        ImGui::Text("BuLang | %s%sLines: %d | Cursor: %d:%d | %s | Tabs: %d | Theme: %s | Zoom: %d%% | %s",
                    breadcrumb.empty() ? "" : breadcrumb.c_str(),
                    breadcrumb.empty() ? "" : " | ",
                    at->editor.GetLineCount(),
                    cursor_line + 1,
                    cursor_column + 1,
                    dirty ? "Modified" : "Saved",
                    tab_count,
                    theme_name,
                    static_cast<int>(at->editor.GetFontScale() * 100.0f),
                    status.c_str());
        ImGui::End();

        ImGui::Render();
        int display_w = 0;
        int display_h = 0;
        SDL_GL_GetDrawableSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.09f, 0.10f, 0.12f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (gif_recorder.IsRecording())
        {
            gif_recorder.CaptureFrame(display_w, display_h, status);
        }
        SDL_GL_SwapWindow(window);
        UpdateWindowTitle(window, at->file_path, dirty, gif_recorder.IsRecording());
    }

    if (gif_recorder.IsRecording())
    {
        gif_recorder.Stop(status);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
