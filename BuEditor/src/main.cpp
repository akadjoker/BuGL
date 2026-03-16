#include "DocumentSymbols.h"
#include "GifRecorder.h"
#include "ImGuiFileDialog.h"
#include "ImGuiConsole.h"
#include "ImGuiFontAwesome.h"
#include "ImGuiSplitter.h"
#include "TextEditor.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_stdlib.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

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
    EditorFontChoice font_choice = EditorFontChoice::DroidSans;
    std::string font_path;
    float font_scale = 1.0f;
    std::string last_file_path = "scripts/vm_cheatsheet.bu";
    std::string bugl_path;
    std::string bytecode_output_path;
    bool outline_visible = true;
    OutlineSide outline_side = OutlineSide::Left;
};

std::string gSettingsPath;

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
    std::deque<DocumentSymbol> symbols;
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
    FILE* file = std::fopen(gSettingsPath.c_str(), "rb");
    if (!file)
    {
        return false;
    }

    char buffer[4096];
    rapidjson::FileReadStream stream(file, buffer, sizeof(buffer));
    rapidjson::Document document;
    document.ParseStream(stream);
    std::fclose(file);

    if (!document.IsObject())
    {
        return false;
    }

    if (document.HasMember("autosave_enabled") && document["autosave_enabled"].IsBool())
    {
        settings.autosave_enabled = document["autosave_enabled"].GetBool();
    }
    if (document.HasMember("autosave_interval_ms") && document["autosave_interval_ms"].IsInt())
    {
        settings.autosave_interval_ms = std::clamp(document["autosave_interval_ms"].GetInt(), 250, 30000);
    }
    if (document.HasMember("console_visible") && document["console_visible"].IsBool())
    {
        settings.console_visible = document["console_visible"].GetBool();
    }
    if (document.HasMember("palette") && document["palette"].IsString())
    {
        settings.palette = PaletteIdFromString(document["palette"].GetString());
    }
    if (document.HasMember("font_choice") && document["font_choice"].IsString())
    {
        settings.font_choice = FontChoiceFromString(document["font_choice"].GetString());
    }
    if (document.HasMember("font_path") && document["font_path"].IsString())
    {
        settings.font_path = document["font_path"].GetString();
    }
    if (document.HasMember("font_scale") && document["font_scale"].IsNumber())
    {
        settings.font_scale = std::clamp(document["font_scale"].GetFloat(), 0.6f, 2.5f);
    }
    if (document.HasMember("last_file_path") && document["last_file_path"].IsString())
    {
        settings.last_file_path = document["last_file_path"].GetString();
    }
    if (document.HasMember("bugl_path") && document["bugl_path"].IsString())
    {
        settings.bugl_path = document["bugl_path"].GetString();
    }
    if (document.HasMember("bytecode_output_path") && document["bytecode_output_path"].IsString())
    {
        settings.bytecode_output_path = document["bytecode_output_path"].GetString();
    }
    if (document.HasMember("outline_visible") && document["outline_visible"].IsBool())
    {
        settings.outline_visible = document["outline_visible"].GetBool();
    }
    if (document.HasMember("outline_side") && document["outline_side"].IsString())
    {
        settings.outline_side = OutlineSideFromString(document["outline_side"].GetString());
    }

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

    FILE* file = std::fopen(gSettingsPath.c_str(), "wb");
    if (!file)
    {
        return false;
    }

    char buffer[4096];
    rapidjson::FileWriteStream stream(file, buffer, sizeof(buffer));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(stream);

    writer.StartObject();
    writer.Key("autosave_enabled");
    writer.Bool(settings.autosave_enabled);
    writer.Key("autosave_interval_ms");
    writer.Int(settings.autosave_interval_ms);
    writer.Key("console_visible");
    writer.Bool(settings.console_visible);
    writer.Key("palette");
    writer.String(PaletteIdToString(settings.palette));
    writer.Key("font_choice");
    writer.String(FontChoiceToString(settings.font_choice));
    writer.Key("font_path");
    writer.String(settings.font_path.c_str());
    writer.Key("font_scale");
    writer.Double(settings.font_scale);
    writer.Key("last_file_path");
    writer.String(settings.last_file_path.c_str());
    writer.Key("bugl_path");
    writer.String(settings.bugl_path.c_str());
    writer.Key("bytecode_output_path");
    writer.String(settings.bytecode_output_path.c_str());
    writer.Key("outline_visible");
    writer.Bool(settings.outline_visible);
    writer.Key("outline_side");
    writer.String(OutlineSideToString(settings.outline_side));
    writer.EndObject();

    std::fclose(file);
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
    gSettingsPath = NormalizePath(project_dir / "BuEditor" / "settings.json");

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
    ImFont* default_editor_font = io.Fonts->AddFontDefault();
    editor_fonts.push_back({"default", "Default", default_editor_font});
    ImFont* droid_sans_font = nullptr;
    const std::string droid_sans_path = ResolveExistingPath("vendor/recastnavigation/RecastDemo/Bin/DroidSans.ttf", executable_dir);
    if (FileExists(droid_sans_path))
    {
        droid_sans_font = io.Fonts->AddFontFromFileTTF(droid_sans_path.c_str(), 18.0f);
        if (droid_sans_font != nullptr)
        {
            editor_fonts.push_back({"droid_sans", "Vendor: Droid Sans", droid_sans_font});
        }
    }

    for (const auto& [font_path, label] : discovered_fonts)
    {
        ImFont* font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 18.0f);
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

    TextEditor editor;
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
        settings.font_choice = EditorFontChoice::Default;
        settings.font_path.clear();
    }
    if (settings.font_choice == EditorFontChoice::Custom && find_font_entry(settings.font_path) == nullptr)
    {
        settings.font_choice = EditorFontChoice::Default;
        settings.font_path.clear();
    }
    SaveSettings(settings);

    editor.SetPalette(settings.palette);
    editor.SetLanguageDefinition(TextEditor::LanguageDefinitionId::BuLang);
    editor.SetShowWhitespacesEnabled(false);
    editor.SetShortTabsEnabled(true);
    editor.SetTabSize(4);
    editor.SetAutoIndentEnabled(true);
    editor.SetFontScale(settings.font_scale);

    bool done = false;
    std::string file_path = ResolveExistingPath(settings.last_file_path, executable_dir);
    std::string bugl_path = ResolveExistingPath(settings.bugl_path, executable_dir);
    std::string bytecode_output_path = settings.bytecode_output_path.empty()
        ? ResolveWritablePath(GetDefaultBytecodePath(file_path), executable_dir)
        : ResolveWritablePath(settings.bytecode_output_path, executable_dir);
    std::string status = "Ready";
    std::string command_output;
    std::string last_command_label;
    std::string last_command_line;
    std::string last_saved_text;
    std::string last_seen_text;
    EditorFontChoice font_choice = settings.font_choice;
    Uint32 last_edit_ticks = SDL_GetTicks();
    Uint32 last_autosave_ticks = 0;
    ImGuiFileDialog file_dialog;
    GifRecorder gif_recorder;
    ImGuiConsole console;
    FindReplaceState find_replace;
    GoToLineState go_to_line;
    FontPickerState font_picker;
    OutlineState outline;
    outline.visible = settings.outline_visible;
    outline.side = settings.outline_side;
    console.SetVisible(settings.console_visible);
    auto command_state = std::make_shared<AsyncCommandState>();

    auto apply_palette = [&](TextEditor::PaletteId palette, const char* label)
    {
        settings.palette = palette;
        editor.SetPalette(palette);
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
        editor.SetFontScale(editor.GetFontScale() + delta);
        settings.font_scale = editor.GetFontScale();
        SaveSettings(settings);
        status = "Zoom: " + std::to_string(static_cast<int>(editor.GetFontScale() * 100.0f)) + "%";
    };
    auto sync_paths_to_settings = [&]()
    {
        settings.last_file_path = file_path;
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
        const int clamped_line = std::max(0, std::min(line, editor.GetLineCount() - 1));
        editor.SetCursorPosition(clamped_line, 0);
        editor.SetViewAtLine(clamped_line, TextEditor::SetViewAtLineMode::Centered);
        status = "Line " + std::to_string(clamped_line + 1);
    };
    auto find_next = [&](bool forward)
    {
        const std::string current_text = editor.GetText();
        if (find_replace.find_text.empty() || current_text.empty())
        {
            status = "Find text is empty";
            return false;
        }

        const std::vector<size_t> line_offsets = BuildLineOffsets(current_text);
        const TextEditor::SelectionPosition selection = editor.GetSelectionPosition();
        const bool has_selection =
            selection.start.line >= 0 &&
            (selection.start.line != selection.end.line || selection.start.column != selection.end.column);
        const TextEditor::TextPosition cursor = editor.GetCursorPosition();
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
        editor.SetSelectionPosition({start, end});
        editor.SetViewAtLine(start.line, TextEditor::SetViewAtLineMode::Centered);
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

        std::string current_text = editor.GetText();
        const TextEditor::SelectionPosition selection = editor.GetSelectionPosition();
        const std::string selected_text = editor.GetSelectedText();
        if (selected_text.empty() || !SelectionMatchesQuery(selected_text, find_replace.find_text, find_replace.case_sensitive))
        {
            if (!find_next(true))
            {
                return false;
            }
            current_text = editor.GetText();
        }

        const std::vector<size_t> line_offsets = BuildLineOffsets(current_text);
        const TextEditor::SelectionPosition updated_selection = editor.GetSelectionPosition();
        const size_t start_offset = LineColumnToOffset(line_offsets, updated_selection.start.line, updated_selection.start.column);
        const size_t end_offset = LineColumnToOffset(line_offsets, updated_selection.end.line, updated_selection.end.column);
        current_text.replace(start_offset, end_offset - start_offset, find_replace.replace_text);
        editor.SetText(current_text);

        const std::vector<size_t> new_offsets = BuildLineOffsets(current_text);
        const TextEditor::TextPosition new_start = OffsetToTextPosition(new_offsets, start_offset);
        const TextEditor::TextPosition new_end = OffsetToTextPosition(new_offsets, start_offset + find_replace.replace_text.size());
        editor.SetSelectionPosition({new_start, new_end});
        editor.SetViewAtLine(new_start.line, TextEditor::SetViewAtLineMode::Centered);
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

        const std::string source_text = editor.GetText();
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

        editor.SetText(result);
        status = "Replaced " + std::to_string(replaced_count) + " matches";
        return true;
    };

    auto run_new = [&]()
    {
        editor.SetText("");
        last_saved_text.clear();
        last_seen_text.clear();
        last_edit_ticks = SDL_GetTicks();
        status = "New file";
    };
    auto open_open_popup = [&]()
    {
        std::filesystem::path start_directory = GetProjectDirectory(executable_dir) / "scripts";
        if (!file_path.empty())
        {
            start_directory = std::filesystem::path(ResolveExistingPath(file_path, executable_dir)).parent_path();
        }
        file_dialog.Open(ImGuiFileDialog::Mode::OpenFile,
                         start_directory,
                         std::filesystem::path(file_path).filename().string());
    };
    auto open_save_as_popup = [&]()
    {
        std::filesystem::path start_directory = GetProjectDirectory(executable_dir) / "scripts";
        const std::string seed_path = file_path.empty() ? settings.last_file_path : file_path;
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
        if (file_path.empty())
        {
            status = "Set a file path before opening.";
            return;
        }

        std::string loaded_text;
        const std::string resolved_path = ResolveExistingPath(file_path, executable_dir);
        if (LoadTextFile(resolved_path, loaded_text, status))
        {
            file_path = resolved_path;
            editor.SetText(loaded_text);
            last_saved_text = loaded_text;
            last_seen_text = loaded_text;
            outline.symbols = ScanDocumentSymbols(loaded_text);
            settings.last_file_path = file_path;
            bytecode_output_path = ResolveWritablePath(GetDefaultBytecodePath(file_path), executable_dir);
            settings.bytecode_output_path = bytecode_output_path;
            SaveSettings(settings);
        }
    };
    auto run_save = [&]()
    {
        if (file_path.empty())
        {
            status = "Set a file path before saving.";
            return;
        }

        const std::string resolved_path = ResolveWritablePath(file_path, executable_dir);
        const std::string current_text = editor.GetText();
        if (SaveTextFile(resolved_path, current_text, status))
        {
            file_path = resolved_path;
            last_saved_text = current_text;
            last_seen_text = current_text;
            last_autosave_ticks = SDL_GetTicks();
            settings.last_file_path = file_path;
            SaveSettings(settings);
        }
    };
    auto ensure_script_ready = [&]() -> bool
    {
        if (file_path.empty())
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

        file_path = ResolveWritablePath(file_path, executable_dir);
        const std::string current_text = editor.GetText();
        if (current_text != last_saved_text)
        {
            run_save();
            if (editor.GetText() != last_saved_text)
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
            QuoteCommandArgument(bugl_path) + " " + QuoteCommandArgument(file_path);
        launch_command("Run Script", command);
    };
    auto run_compile_bytecode = [&]()
    {
        if (!ensure_script_ready())
        {
            return;
        }

        bytecode_output_path = ResolveWritablePath(
            bytecode_output_path.empty() ? GetDefaultBytecodePath(file_path) : bytecode_output_path,
            executable_dir);

        sync_paths_to_settings();
        const std::string command =
            QuoteCommandArgument(bugl_path) + " --compile-bc " +
            QuoteCommandArgument(file_path) + " " +
            QuoteCommandArgument(bytecode_output_path);
        launch_command("Compile Bytecode", command);
    };

    if (!file_path.empty())
    {
        run_open();
    }

    while (!done)
    {
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
                done = true;
            }
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
            {
                const bool ctrl_down = (event.key.keysym.mod & KMOD_CTRL) != 0;
                if (ctrl_down)
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
                            if (file_path.empty())
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
                        find_replace.visible = true;
                        find_replace.replace_visible = false;
                        find_replace.focus_find_input = true;
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
                        editor.SetFontScale(1.0f);
                        settings.font_scale = 1.0f;
                        SaveSettings(settings);
                        status = "Zoom reset";
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
                    if (file_path.empty())
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
                if (ImGui::MenuItem("Autosave", nullptr, settings.autosave_enabled))
                {
                    settings.autosave_enabled = !settings.autosave_enabled;
                    SaveSettings(settings);
                    status = settings.autosave_enabled ? "Autosave enabled" : "Autosave disabled";
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
                    editor.SetFontScale(1.0f);
                    settings.font_scale = 1.0f;
                    SaveSettings(settings);
                    status = "Zoom reset";
                }
                ImGui::Separator();
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
                if (ImGui::MenuItem("VSCode Dark", nullptr, editor.GetPalette() == TextEditor::PaletteId::VsCodeDark))
                {
                    apply_palette(TextEditor::PaletteId::VsCodeDark, "VSCode Dark");
                }
                if (ImGui::MenuItem("Mariana", nullptr, editor.GetPalette() == TextEditor::PaletteId::Mariana))
                {
                    apply_palette(TextEditor::PaletteId::Mariana, "Mariana");
                }
                if (ImGui::MenuItem("Dark", nullptr, editor.GetPalette() == TextEditor::PaletteId::Dark))
                {
                    apply_palette(TextEditor::PaletteId::Dark, "Dark");
                }
                if (ImGui::MenuItem("Light", nullptr, editor.GetPalette() == TextEditor::PaletteId::Light))
                {
                    apply_palette(TextEditor::PaletteId::Light, "Light");
                }
                if (ImGui::MenuItem("Retro Blue", nullptr, editor.GetPalette() == TextEditor::PaletteId::RetroBlue))
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
                file_path = result.path.string();
                if (result.mode == ImGuiFileDialog::Mode::OpenFile)
                {
                    run_open();
                }
                else if (result.mode == ImGuiFileDialog::Mode::SaveFile)
                {
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
        const std::string current_text = editor.GetText();
        if (current_text != last_seen_text)
        {
            outline.symbols = ScanDocumentSymbols(current_text);
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
        const float editor_region_height = std::max(80.0f, ImGui::GetContentRegionAvail().y - footer_height);
        int cursor_line = 0;
        int cursor_column = 0;
        editor.GetCursorPosition(cursor_line, cursor_column);
        const auto select_symbol_block = [&](const DocumentSymbol& symbol)
        {
            TextEditor::SelectionPosition selection;
            selection.start.line = symbol.line;
            selection.start.column = 0;
            selection.end.line = symbol.end_line;
            selection.end.column = std::numeric_limits<int>::max();
            editor.SetSelectionPosition(selection);
            editor.SetViewAtLine(symbol.line, TextEditor::SetViewAtLineMode::Centered);
            status = "Selected " + symbol.name;
        };
        const DocumentSymbol* active_symbol = FindInnermostSymbolAtLine(outline.symbols, cursor_line);

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
            render_symbols(render_symbols, outline.symbols);
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

        const float editor_width = (outline.visible && outline.side == OutlineSide::Right)
            ? std::max(80.0f, ImGui::GetContentRegionAvail().x - outline.width - 6.0f)
            : -1.0f;
        ImGui::PushFont(selected_font());
        editor.Render("##source", parent_focused, ImVec2(editor_width, editor_region_height), false);
        ImGui::PopFont();

        if (outline.visible && outline.side == OutlineSide::Right)
        {
            ImGui::SameLine(0.0f, 0.0f);
            ImGuiSplitter::VerticalRight("outline_splitter_right", &outline.width, 260.0f, 160.0f, editor_region_height);
            ImGui::SameLine(0.0f, 0.0f);
            render_outline_panel();
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && io.KeyCtrl && io.MouseWheel != 0.0f)
        {
            adjust_zoom(io.MouseWheel > 0.0f ? 0.1f : -0.1f);
        }
        const bool dirty = current_text != last_saved_text;
        if (current_text != last_seen_text)
        {
            last_edit_ticks = SDL_GetTicks();
            last_seen_text = current_text;
        }

        if (settings.autosave_enabled && dirty)
        {
            const Uint32 now = SDL_GetTicks();
            if (now - last_autosave_ticks >= static_cast<Uint32>(settings.autosave_interval_ms) &&
                now - last_edit_ticks >= static_cast<Uint32>(settings.autosave_interval_ms))
            {
                run_save();
                status = "Autosaved";
            }
        }
        const char* theme_name =
            editor.GetPalette() == TextEditor::PaletteId::VsCodeDark ? "VSCode Dark" :
            editor.GetPalette() == TextEditor::PaletteId::Mariana ? "Mariana" :
            editor.GetPalette() == TextEditor::PaletteId::Light ? "Light" :
            editor.GetPalette() == TextEditor::PaletteId::RetroBlue ? "Retro Blue" : "Dark";
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

        ImGui::Separator();
        ImGui::Text("BuLang | Theme: %s | Font: %s | Zoom: %d%% | Autosave: %s | GIF: %s | Tool: %s | Lines: %d | Cursor: %d:%d | %s | Status: %s",
                    theme_name,
                    font_name.c_str(),
                    static_cast<int>(editor.GetFontScale() * 100.0f),
                    settings.autosave_enabled ? "On" : "Off",
                    gif_state,
                    command_running ? "Running" : "Idle",
                    editor.GetLineCount(),
                    cursor_line + 1,
                    cursor_column + 1,
                    dirty ? "Modified" : "Saved",
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
        UpdateWindowTitle(window, file_path, dirty, gif_recorder.IsRecording());
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
