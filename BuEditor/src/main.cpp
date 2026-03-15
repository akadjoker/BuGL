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
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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
    DroidSans
};

struct EditorSettings
{
    bool autosave_enabled = false;
    int autosave_interval_ms = 1500;
    bool console_visible = false;
    TextEditor::PaletteId palette = TextEditor::PaletteId::VsCodeDark;
    EditorFontChoice font_choice = EditorFontChoice::DroidSans;
    float font_scale = 1.0f;
    std::string last_file_path = "scripts/vm_cheatsheet.bu";
    std::string bugl_path;
    std::string bytecode_output_path;
};

const char* kSettingsPath = "BuEditor/settings.json";

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

struct FileDialogEntry
{
    std::string label;
    std::filesystem::path path;
    bool is_directory = false;
};

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

std::filesystem::path GetDialogStartDirectory(const std::string& candidate_path, const std::filesystem::path& executable_dir)
{
    const std::filesystem::path project_dir = GetProjectDirectory(executable_dir);
    const std::filesystem::path scripts_dir = project_dir / "scripts";
    const std::string resolved = ResolveExistingPath(candidate_path, executable_dir);
    std::error_code ec;

    if (!resolved.empty())
    {
        const std::filesystem::path resolved_path(resolved);
        if (std::filesystem::is_directory(resolved_path, ec))
        {
            return resolved_path;
        }
        ec.clear();
        if (std::filesystem::exists(resolved_path, ec))
        {
            return resolved_path.parent_path();
        }

        const std::filesystem::path parent = resolved_path.parent_path();
        ec.clear();
        if (!parent.empty() && std::filesystem::exists(parent, ec))
        {
            return parent;
        }
    }

    ec.clear();
    if (std::filesystem::exists(scripts_dir, ec))
    {
        return scripts_dir;
    }

    return project_dir;
}

std::string GetDialogFileName(const std::string& candidate_path)
{
    return std::filesystem::path(candidate_path).filename().string();
}

std::vector<FileDialogEntry> ListDirectoryEntries(const std::filesystem::path& directory)
{
    std::vector<FileDialogEntry> entries;
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec))
    {
        return entries;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory, std::filesystem::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            break;
        }

        std::error_code type_error;
        const bool is_directory = entry.is_directory(type_error);
        FileDialogEntry dialog_entry;
        dialog_entry.path = entry.path();
        dialog_entry.label = dialog_entry.path.filename().string();
        dialog_entry.is_directory = !type_error && is_directory;
        entries.push_back(dialog_entry);
    }

    std::sort(entries.begin(), entries.end(), [](const FileDialogEntry& lhs, const FileDialogEntry& rhs)
    {
        if (lhs.is_directory != rhs.is_directory)
        {
            return lhs.is_directory > rhs.is_directory;
        }
        return lhs.label < rhs.label;
    });

    return entries;
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
    }
    return "default";
}

EditorFontChoice FontChoiceFromString(const std::string& value)
{
    if (value == "droid_sans")
    {
        return EditorFontChoice::DroidSans;
    }
    return EditorFontChoice::Default;
}

bool LoadSettings(EditorSettings& settings)
{
    FILE* file = std::fopen(kSettingsPath, "rb");
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
    FILE* file = std::fopen(kSettingsPath, "wb");
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
    writer.Key("font_scale");
    writer.Double(settings.font_scale);
    writer.Key("last_file_path");
    writer.String(settings.last_file_path.c_str());
    writer.Key("bugl_path");
    writer.String(settings.bugl_path.c_str());
    writer.Key("bytecode_output_path");
    writer.String(settings.bytecode_output_path.c_str());
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

void UpdateWindowTitle(SDL_Window* window, const std::string& file_path, bool dirty)
{
    std::string title = "BuEditor";
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
enum class FileDialogMode
{
    OpenFile,
    SaveFile
};

    const std::filesystem::path executable_dir = GetExecutableDirectory(argc > 0 ? argv[0] : nullptr);
    const std::filesystem::path project_dir = GetProjectDirectory(executable_dir);

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

    ImFont* default_editor_font = io.Fonts->AddFontDefault();
    ImFont* droid_sans_font = nullptr;
    const std::string droid_sans_path = "vendor/recastnavigation/RecastDemo/Bin/DroidSans.ttf";
    if (FileExists(droid_sans_path))
    {
        droid_sans_font = io.Fonts->AddFontFromFileTTF(droid_sans_path.c_str(), 18.0f);
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

    if (settings.font_choice == EditorFontChoice::DroidSans && droid_sans_font == nullptr)
    {
        settings.font_choice = EditorFontChoice::Default;
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
    bool console_visible = settings.console_visible;
    bool file_dialog_requested = false;
    bool dialog_focus_filename = false;
    FileDialogMode file_dialog_mode = FileDialogMode::OpenFile;
    std::filesystem::path dialog_directory = GetDialogStartDirectory(file_path, executable_dir);
    std::string dialog_file_name = GetDialogFileName(file_path);
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
        if (font_choice == EditorFontChoice::DroidSans && droid_sans_font != nullptr)
        {
            return droid_sans_font;
        }
        return default_editor_font;
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
        console_visible = true;
        settings.console_visible = true;
        SaveSettings(settings);

        auto shared_state = command_state;
        std::thread([shared_state, label, command]()
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
        file_dialog_mode = FileDialogMode::OpenFile;
        dialog_directory = GetDialogStartDirectory(file_path, executable_dir);
        dialog_file_name = GetDialogFileName(file_path);
        file_dialog_requested = true;
        dialog_focus_filename = true;
    };
    auto open_save_as_popup = [&]()
    {
        file_dialog_mode = FileDialogMode::SaveFile;
        dialog_directory = GetDialogStartDirectory(file_path, executable_dir);
        dialog_file_name = GetDialogFileName(file_path.empty() ? settings.last_file_path : file_path);
        file_dialog_requested = true;
        dialog_focus_filename = true;
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

    run_open();

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
                    console_visible = !console_visible;
                    settings.console_visible = console_visible;
                    SaveSettings(settings);
                    status = console_visible ? "Console shown" : "Console hidden";
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
                if (ImGui::MenuItem("Console", "F8", console_visible))
                {
                    console_visible = !console_visible;
                    settings.console_visible = console_visible;
                    SaveSettings(settings);
                    status = console_visible ? "Console shown" : "Console hidden";
                }
                if (ImGui::MenuItem("Clear Output"))
                {
                    command_output.clear();
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
                if (ImGui::MenuItem("Console", "F8", console_visible))
                {
                    console_visible = !console_visible;
                    settings.console_visible = console_visible;
                    SaveSettings(settings);
                    status = console_visible ? "Console shown" : "Console hidden";
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
                if (ImGui::MenuItem("Default Font", nullptr, font_choice == EditorFontChoice::Default))
                {
                    font_choice = EditorFontChoice::Default;
                    settings.font_choice = font_choice;
                    SaveSettings(settings);
                    status = "Font: Default";
                }
                if (droid_sans_font != nullptr &&
                    ImGui::MenuItem("Droid Sans", nullptr, font_choice == EditorFontChoice::DroidSans))
                {
                    font_choice = EditorFontChoice::DroidSans;
                    settings.font_choice = font_choice;
                    SaveSettings(settings);
                    status = "Font: Droid Sans";
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

        if (file_dialog_requested)
        {
            ImGui::OpenPopup("File Browser");
            file_dialog_requested = false;
        }

        if (ImGui::BeginPopupModal("File Browser", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const std::filesystem::path project_dir = GetProjectDirectory(executable_dir);
            const std::filesystem::path scripts_dir = project_dir / "scripts";
            const std::filesystem::path bin_dir = executable_dir;

            if (ImGui::Button("Project"))
            {
                dialog_directory = project_dir;
            }
            ImGui::SameLine();
            if (ImGui::Button("Scripts"))
            {
                dialog_directory = scripts_dir;
            }
            ImGui::SameLine();
            if (ImGui::Button("Bin"))
            {
                dialog_directory = bin_dir;
            }
            ImGui::SameLine();
            if (ImGui::Button("Up"))
            {
                const std::filesystem::path parent = dialog_directory.parent_path();
                if (!parent.empty())
                {
                    dialog_directory = parent;
                }
            }

            ImGui::Separator();
            ImGui::TextWrapped("%s", dialog_directory.string().c_str());

            const std::vector<FileDialogEntry> entries = ListDirectoryEntries(dialog_directory);
            ImGui::BeginChild("##file_dialog_entries", ImVec2(680.0f, 280.0f), true);
            for (const FileDialogEntry& entry : entries)
            {
                const std::string item_label = entry.is_directory
                    ? "[DIR] " + entry.label
                    : entry.label;
                const bool is_selected = dialog_file_name == entry.label;
                if (ImGui::Selectable(item_label.c_str(), is_selected))
                {
                    if (entry.is_directory)
                    {
                        dialog_directory = entry.path;
                    }
                    else
                    {
                        dialog_file_name = entry.label;
                    }
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (entry.is_directory)
                    {
                        dialog_directory = entry.path;
                    }
                    else
                    {
                        dialog_file_name = entry.label;
                        if (file_dialog_mode == FileDialogMode::OpenFile)
                        {
                            file_path = (dialog_directory / dialog_file_name).string();
                            run_open();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::SetNextItemWidth(680.0f);
            ImGui::InputTextWithHint(
                "##dialog_file_name",
                file_dialog_mode == FileDialogMode::OpenFile ? "File to open" : "File to save",
                &dialog_file_name);
            if (dialog_focus_filename)
            {
                ImGui::SetKeyboardFocusHere(-1);
                dialog_focus_filename = false;
            }

            const bool has_target_file = !dialog_file_name.empty();
            if (!has_target_file)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button(file_dialog_mode == FileDialogMode::OpenFile ? "Open" : "Save", ImVec2(100.0f, 0.0f)))
            {
                file_path = (dialog_directory / dialog_file_name).string();
                if (file_dialog_mode == FileDialogMode::OpenFile)
                {
                    run_open();
                }
                else
                {
                    run_save();
                }
                ImGui::CloseCurrentPopup();
            }
            if (!has_target_file)
            {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
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
        if (command_running)
        {
            console_visible = true;
        }
        settings.console_visible = console_visible;
        const bool show_output_panel = console_visible;

        const float output_panel_height = show_output_panel ? 160.0f : 0.0f;
        const float footer_height = show_output_panel
            ? output_panel_height + (ImGui::GetFrameHeightWithSpacing() * 5.0f)
            : ImGui::GetFrameHeightWithSpacing() * 2.0f;
        const bool parent_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        ImGui::PushFont(selected_font());
        editor.Render("##source", parent_focused, ImVec2(-1.0f, -footer_height), false);
        ImGui::PopFont();

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && io.KeyCtrl && io.MouseWheel != 0.0f)
        {
            adjust_zoom(io.MouseWheel > 0.0f ? 0.1f : -0.1f);
        }

        const std::string current_text = editor.GetText();
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
        UpdateWindowTitle(window, file_path, dirty);

        int cursor_line = 0;
        int cursor_column = 0;
        editor.GetCursorPosition(cursor_line, cursor_column);
        const char* theme_name =
            editor.GetPalette() == TextEditor::PaletteId::VsCodeDark ? "VSCode Dark" :
            editor.GetPalette() == TextEditor::PaletteId::Mariana ? "Mariana" :
            editor.GetPalette() == TextEditor::PaletteId::Light ? "Light" :
            editor.GetPalette() == TextEditor::PaletteId::RetroBlue ? "Retro Blue" : "Dark";
        const char* font_name = font_choice == EditorFontChoice::DroidSans ? "Droid Sans" : "Default";

        if (show_output_panel)
        {
            ImGui::Separator();
            ImGui::Text("Console%s", command_running ? " [running]" : "");
            if (!last_command_label.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("%s", last_command_label.c_str());
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Hide"))
            {
                console_visible = false;
                settings.console_visible = false;
                SaveSettings(settings);
                status = "Console hidden";
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear"))
            {
                command_output.clear();
                std::lock_guard<std::mutex> lock(command_state->mutex);
                command_state->output.clear();
                last_command_label.clear();
                last_command_line.clear();
                status = "Output cleared";
            }
            ImGui::BeginChild("##bugl_output", ImVec2(-1.0f, output_panel_height), true, ImGuiWindowFlags_HorizontalScrollbar);
            if (command_output.empty())
            {
                ImGui::TextUnformatted("No messages.");
            }
            else
            {
                ImGui::TextUnformatted(command_output.c_str());
            }
            ImGui::EndChild();
        }

        ImGui::Separator();
        ImGui::Text("BuLang | Theme: %s | Font: %s | Zoom: %d%% | Autosave: %s | Tool: %s | Lines: %d | Cursor: %d:%d | %s | Status: %s",
                    theme_name,
                    font_name,
                    static_cast<int>(editor.GetFontScale() * 100.0f),
                    settings.autosave_enabled ? "On" : "Off",
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
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
