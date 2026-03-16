#include "ImGuiConsole.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFontAwesome.h"
#include "ImGuiPopup.h"
#include "ImGuiPropertyGrid.h"
#include "ImGuiSplitter.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
struct DemoObject
{
    std::string name;
    float position[2];
    float size[2];
    float rotation = 0.0f;
    float opacity = 1.0f;
    float tint[4];
    int layer = 0;
    int material = 0;
    int shape = 0;
    bool visible = true;
    bool locked = false;
    bool trigger = false;
};

std::filesystem::path FindWorkspaceRoot(const std::filesystem::path& start)
{
    std::error_code ec;
    std::filesystem::path current = start;

    while (!current.empty())
    {
        const bool has_root_cmake = std::filesystem::exists(current / "CMakeLists.txt", ec);
        ec.clear();
        const bool has_bueditor = std::filesystem::exists(current / "BuEditor", ec);
        ec.clear();
        const bool has_scripts = std::filesystem::exists(current / "scripts", ec);
        ec.clear();
        if (has_root_cmake && has_bueditor && has_scripts)
        {
            return current.lexically_normal();
        }

        const std::filesystem::path parent = current.parent_path();
        if (parent == current)
        {
            break;
        }
        current = parent;
    }

    return start.lexically_normal();
}

std::filesystem::path GetProjectDirectory(const char* argv0)
{
    std::error_code ec;
    if (argv0 && argv0[0] != '\0')
    {
        const std::filesystem::path executable = std::filesystem::absolute(argv0, ec);
        if (!ec && !executable.empty())
        {
            return FindWorkspaceRoot(executable.parent_path());
        }
    }

    return std::filesystem::current_path(ec).lexically_normal();
}
}

int main(int argc, char** argv)
{
    const std::filesystem::path project_dir = GetProjectDirectory(argc > 0 ? argv[0] : nullptr);
    const std::filesystem::path scripts_dir = project_dir / "scripts";
    const std::filesystem::path bin_dir = project_dir / "bin";

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
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "BuWidgets Demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        820,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
    {
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
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
    io.Fonts->AddFontDefault();
    ImGuiFontAwesome::MergeSolid(io, 13.0f);

    if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context))
    {
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    ImGuiFileDialog file_dialog;
    ImGuiConsole console;
    ImGuiPopup popup;
    console.SetVisible(true);

    std::string selected_file = (scripts_dir / "vm_cheatsheet.bu").string();
    std::string selected_folder = scripts_dir.string();
    std::string status = "Ready";
    float hierarchy_width = 230.0f;
    float inspector_width = 300.0f;
    float top_height = 560.0f;
    std::vector<DemoObject> objects = {
        {"PlayerSpawn", {120.0f, 110.0f}, {80.0f, 48.0f}, 0.0f, 1.0f, {0.36f, 0.74f, 0.45f, 1.0f}, 1, 0, 0, true, false, false},
        {"Crate_A", {290.0f, 210.0f}, {64.0f, 64.0f}, 0.0f, 1.0f, {0.74f, 0.58f, 0.35f, 1.0f}, 2, 1, 1, true, false, false},
        {"LampPost", {420.0f, 120.0f}, {30.0f, 120.0f}, 0.0f, 0.85f, {0.78f, 0.76f, 0.52f, 1.0f}, 0, 2, 0, true, false, false},
        {"PortalExit", {560.0f, 260.0f}, {96.0f, 120.0f}, 0.0f, 1.0f, {0.40f, 0.66f, 0.92f, 1.0f}, 3, 3, 2, true, false, true},
    };
    int selected_index = 0;

    auto append_log = [&](const std::string& text)
    {
        std::string combined = console.GetText();
        if (!combined.empty() && combined.back() != '\n')
        {
            combined += '\n';
        }
        combined += text;
        console.SetText(combined);
    };

    append_log("\x1b[0;36m[21:02:40]\x1b[0m \x1b[1;32minfo \x1b[0m Widgets demo booted");

    bool done = false;
    while (!done)
    {
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
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("BuWidgetsDemo", nullptr,
                     ImGuiWindowFlags_MenuBar |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse);

        file_dialog.Render(project_dir, scripts_dir, bin_dir);
        if (file_dialog.HasResult())
        {
            const ImGuiFileDialog::Result result = file_dialog.ConsumeResult();
            if (result.accepted)
            {
                if (result.mode == ImGuiFileDialog::Mode::ChooseFolder)
                {
                    selected_folder = result.path.string();
                    status = "Folder selected";
                    append_log("\x1b[0;36m[21:02:41]\x1b[0m \x1b[1;32minfo \x1b[0m Folder: " + selected_folder);
                }
                else
                {
                    selected_file = result.path.string();
                    status = result.mode == ImGuiFileDialog::Mode::OpenFile ? "File selected" : "Save target selected";
                    append_log("\x1b[0;36m[21:02:41]\x1b[0m \x1b[1;32minfo \x1b[0m File: " + selected_file);
                }
            }
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open File"))
                {
                    file_dialog.Open(ImGuiFileDialog::Mode::OpenFile,
                                     scripts_dir,
                                     std::filesystem::path(selected_file).filename().string());
                }
                if (ImGui::MenuItem("Save File"))
                {
                    file_dialog.Open(ImGuiFileDialog::Mode::SaveFile,
                                     scripts_dir,
                                     std::filesystem::path(selected_file).filename().string());
                }
                if (ImGui::MenuItem("Choose Folder"))
                {
                    file_dialog.Open(ImGuiFileDialog::Mode::ChooseFolder, selected_folder);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Add ANSI Log"))
                {
                    append_log("\x1b[0;36m[21:02:42]\x1b[0m \x1b[1;33mwarn \x1b[0m Fake terminal colors from ImGuiConsole");
                }
                if (ImGui::MenuItem("Info Popup"))
                {
                    popup.Open(ImGuiPopup::Kind::Info,
                               "Info",
                               "BuWidgetsDemo now has reusable modal popups for editor actions.");
                }
                if (ImGui::MenuItem("Warning Popup"))
                {
                    popup.Open(ImGuiPopup::Kind::Warning,
                               "Warning",
                               "This action would overwrite the current generated bytecode output.");
                }
                if (ImGui::MenuItem("Error Popup"))
                {
                    popup.Open(ImGuiPopup::Kind::Error,
                               "Error",
                               "Failed to compile script. Check the console output for details.");
                }
                if (ImGui::MenuItem("Confirm Popup"))
                {
                    popup.Open(
                        ImGuiPopup::Kind::Confirm,
                        "Delete Object",
                        "Remove the selected object from the scene?",
                        [&]()
                        {
                            if (selected_index >= 0 && selected_index < static_cast<int>(objects.size()))
                            {
                                const std::string removed_name = objects[selected_index].name;
                                objects.erase(objects.begin() + selected_index);
                                selected_index = objects.empty() ? -1 : std::min(selected_index, static_cast<int>(objects.size()) - 1);
                                status = "Deleted " + removed_name;
                                append_log("\x1b[0;36m[21:02:43]\x1b[0m \x1b[1;32minfo \x1b[0m Deleted object: " + removed_name);
                            }
                        },
                        [&]()
                        {
                            status = "Delete cancelled";
                        });
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        popup.Render();

        const float bottom_reserved = console.IsVisible() ? 190.0f : 44.0f;
        const float working_height = ImGui::GetContentRegionAvail().y - bottom_reserved;
        top_height = std::clamp(top_height, 260.0f, working_height);
        ImGui::BeginChild("##editor_top", ImVec2(-1.0f, top_height), false);

        ImGui::BeginChild("##hierarchy", ImVec2(hierarchy_width, 0.0f), true);
        ImGui::TextUnformatted("Hierarchy");
        ImGui::Separator();
        for (int i = 0; i < static_cast<int>(objects.size()); ++i)
        {
            DemoObject& object = objects[i];
            const bool is_selected = selected_index == i;
            std::string label = object.visible ? object.name : object.name + " (hidden)";
            if (ImGui::Selectable(label.c_str(), is_selected))
            {
                selected_index = i;
                status = "Selected " + object.name;
            }
        }
        ImGui::Separator();
        ImGui::TextDisabled("File: %s", std::filesystem::path(selected_file).filename().string().c_str());
        ImGui::TextDisabled("Folder: %s", std::filesystem::path(selected_folder).filename().string().c_str());
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);
        ImGuiSplitter::Vertical("left_split", &hierarchy_width, 180.0f, 440.0f, -1.0f);
        ImGui::SameLine(0.0f, 0.0f);

        const float center_width = ImGui::GetContentRegionAvail().x - inspector_width - 6.0f;
        ImGui::BeginChild("##scene", ImVec2(center_width, 0.0f), true);
        ImGui::TextUnformatted("Scene");
        ImGui::Separator();
        ImGui::TextDisabled("Drag objects with left mouse. This panel is the base for a future game editor viewport.");

        const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        const ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(24, 28, 34, 255), 6.0f);
        for (float x = 0.0f; x < canvas_size.x; x += 32.0f)
        {
            draw_list->AddLine(ImVec2(canvas_pos.x + x, canvas_pos.y),
                               ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y),
                               IM_COL32(42, 48, 56, 255));
        }
        for (float y = 0.0f; y < canvas_size.y; y += 32.0f)
        {
            draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + y),
                               ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y),
                               IM_COL32(42, 48, 56, 255));
        }

        ImGui::InvisibleButton("##scene_canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
        const bool scene_hovered = ImGui::IsItemHovered();
        if (selected_index >= 0 && selected_index < static_cast<int>(objects.size()))
        {
            DemoObject& selected = objects[selected_index];
            if (scene_hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                selected.position[0] += io.MouseDelta.x;
                selected.position[1] += io.MouseDelta.y;
                status = "Dragging " + selected.name;
            }
        }

        for (int i = 0; i < static_cast<int>(objects.size()); ++i)
        {
            const DemoObject& object = objects[i];
            if (!object.visible)
            {
                continue;
            }

            const ImVec2 rect_min(canvas_pos.x + object.position[0], canvas_pos.y + object.position[1]);
            const ImVec2 rect_max(rect_min.x + object.size[0], rect_min.y + object.size[1]);
            const ImU32 fill = ImGui::ColorConvertFloat4ToU32(ImVec4(object.tint[0], object.tint[1], object.tint[2], object.opacity));
            const ImU32 outline = i == selected_index ? IM_COL32(255, 255, 255, 255) : IM_COL32(10, 12, 15, 255);
            draw_list->AddRectFilled(rect_min, rect_max, fill, 4.0f);
            draw_list->AddRect(rect_min, rect_max, outline, 4.0f, 0, i == selected_index ? 3.0f : 1.0f);
            draw_list->AddText(ImVec2(rect_min.x + 8.0f, rect_min.y + 8.0f), IM_COL32(245, 246, 248, 255), object.name.c_str());
        }
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);
        ImGuiSplitter::VerticalRight("right_split", &inspector_width, 220.0f, 220.0f, -1.0f);
        ImGui::SameLine(0.0f, 0.0f);

        ImGui::BeginChild("##inspector", ImVec2(0.0f, 0.0f), true);
        ImGui::TextUnformatted("Inspector");
        ImGui::Separator();
        if (selected_index >= 0 && selected_index < static_cast<int>(objects.size()))
        {
            DemoObject& selected = objects[selected_index];
            static const char* material_items[] = {"Default", "Metal", "Wood", "Portal"};
            static const char* shape_items[] = {"Rectangle", "Capsule", "Trigger Zone"};

            if (ImGuiPropertyGrid::Section("Transform"))
            {
                if (ImGuiPropertyGrid::Begin("##transform_grid"))
                {
                    ImGuiPropertyGrid::InputText("Name", &selected.name);
                    ImGuiPropertyGrid::DragFloat2("Position", selected.position, 1.0f);
                    ImGuiPropertyGrid::DragFloat2("Size", selected.size, 1.0f, 8.0f, 2048.0f);
                    ImGuiPropertyGrid::DragFloat("Rotation", &selected.rotation, 0.5f, -360.0f, 360.0f);
                    ImGuiPropertyGrid::DragInt("Layer", &selected.layer, 1.0f, 0, 32);
                    ImGuiPropertyGrid::End();
                }
            }

            if (ImGuiPropertyGrid::Section("Render"))
            {
                if (ImGuiPropertyGrid::Begin("##render_grid"))
                {
                    ImGuiPropertyGrid::ColorEdit4("Tint", selected.tint);
                    ImGuiPropertyGrid::SliderFloat("Opacity", &selected.opacity, 0.05f, 1.0f, "%.2f");
                    ImGuiPropertyGrid::Combo("Material", &selected.material, material_items, IM_ARRAYSIZE(material_items));
                    ImGuiPropertyGrid::Checkbox("Visible", &selected.visible);
                    ImGuiPropertyGrid::End();
                }
            }

            if (ImGuiPropertyGrid::Section("Physics"))
            {
                if (ImGuiPropertyGrid::Begin("##physics_grid"))
                {
                    ImGuiPropertyGrid::Combo("Shape", &selected.shape, shape_items, IM_ARRAYSIZE(shape_items));
                    ImGuiPropertyGrid::Checkbox("Locked", &selected.locked);
                    ImGuiPropertyGrid::Checkbox("Trigger", &selected.trigger);
                    ImGuiPropertyGrid::End();
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Duplicate", ImVec2(-1.0f, 0.0f)))
            {
                DemoObject copy = selected;
                copy.name += "_Copy";
                copy.position[0] += 24.0f;
                copy.position[1] += 24.0f;
                objects.push_back(copy);
                selected_index = static_cast<int>(objects.size()) - 1;
                status = "Duplicated " + selected.name;
            }
            if (ImGui::Button("Frame Selection", ImVec2(-1.0f, 0.0f)))
            {
                status = "Framed " + selected.name;
            }
        }
        ImGui::EndChild();
        ImGui::EndChild();

        ImGuiSplitter::Horizontal("bottom_split", &top_height, 260.0f, console.IsVisible() ? 130.0f : 24.0f, -1.0f);

        const float console_height = std::max(120.0f, ImGui::GetContentRegionAvail().y - 8.0f);
        const ImGuiConsole::RenderResult console_result =
            console.Render("ImGuiConsole", "Hierarchy + Scene + Inspector demo", false, console_height);
        if (console_result.hidden)
        {
            status = "Console hidden";
        }
        if (console_result.cleared)
        {
            status = "Console cleared";
        }

        if (!console.IsVisible())
        {
            if (ImGui::Button("Show Console", ImVec2(140.0f, 0.0f)))
            {
                console.SetVisible(true);
                status = "Console shown";
            }
        }

        ImGui::Separator();
        ImGui::Text("Status: %s", status.c_str());

        ImGui::End();

        ImGui::Render();
        int display_w = 0;
        int display_h = 0;
        SDL_GL_GetDrawableSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.09f, 0.10f, 0.12f, 1.0f);
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
