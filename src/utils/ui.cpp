#include "pch.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "core/clock.h"
#include "core/log.h"
#include "core/window.h"
#include "utils/ui.h"
#include "utils/factory.h"

#include "imgui_internal.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl3.h"

#include "imstb_rectpack.h"
#include "imstb_textedit.h"
#include "imstb_truetype.h"

using namespace core;

namespace ui {

    ImFont* truetype_font;  // default font: TrueType, QuickSand-Regular, 17pt
    ImFont* opentype_font;  // reserved font: OpenType, Palatino Linotype, 15pt

    void Init() {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        // load fonts from disk
        ImFontConfig config;
        config.OversampleH = 2;
        config.OversampleV = 1;
        config.GlyphExtraSpacing.x = 0.0f;
        
        truetype_font = io.Fonts->AddFontFromFileTTF((RES + "fonts\\quicksand.ttf").c_str(), 17.0f);
        opentype_font = io.Fonts->AddFontFromFileTTF((RES + "fonts\\palatino.ttf").c_str(), 15.0f, &config);

        // build font textures
        unsigned char* pixels;
        int width, height, bytes_per_pixel;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

        // load default dark theme
        ImGui::StyleColorsDark();

        // setup custom styles
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.TabBorderSize = 0.0f;

        style.ScrollbarSize = 18.0f;
        style.GrabMinSize = 10.0f;

        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(4.0f, 6.0f);
        style.ItemSpacing = ImVec2(10.0f, 10.0f);
        style.ItemInnerSpacing = ImVec2(10.0f, 10.0f);
        style.IndentSpacing = 16.0f;

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 0.0f;
        style.TabRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.ScrollbarRounding = 12.0f;

        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ColorButtonPosition = ImGuiDir_Right;

        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        // setup custom colors
        auto& c = ImGui::GetStyle().Colors;

        c[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.85f);  // normal windows
        c[ImGuiCol_ChildBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.85f);   // child windows
        c[ImGuiCol_PopupBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.85f);   // popups, menus, tooltips

        c[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);  // checkbox, radio button, slider, text input
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 0.75f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 0.75f);

        c[ImGuiCol_TitleBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.75f);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.3f, 0.0f, 0.9f);
        c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);

        c[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.9f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.9f);

        c[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        c[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.4f, 0.0f, 0.9f);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.5f, 0.0f, 0.9f);

        c[ImGuiCol_Button] = ImVec4(0.0f, 0.3f, 0.0f, 0.9f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.55f, 0.0f, 0.9f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.5f, 0.0f, 0.9f);

        c[ImGuiCol_Header] = ImVec4(0.5f, 0.0f, 1.0f, 0.5f);  // collapsing header, tree node, selectable, menu item
        c[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.0f, 1.0f, 0.8f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.5f, 0.0f, 1.0f, 0.7f);

        c[ImGuiCol_Tab] = ImVec4(0.0f, 0.3f, 0.0f, 0.8f);
        c[ImGuiCol_TabHovered] = ImVec4(0.0f, 0.4f, 0.0f, 0.8f);
        c[ImGuiCol_TabActive] = ImVec4(0.0f, 0.4f, 0.0f, 0.8f);
        c[ImGuiCol_TabUnfocused] = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);
        c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);

        ImGui_ImplGLUT_Init();
        ImGui_ImplOpenGL3_Init();
    }

    void Clear() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
    }

    void NewFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGLUT_NewFrame();
    }

    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void DrawVerticalLine() {
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    }

    void DrawTooltip(const char* desc, float spacing) {
        ImGui::SameLine(0.0f, spacing);
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            ImGui::PopStyleColor(2);
        }
    }

    static void DrawAboutWindow(const char* version, bool* show) {
        if (Window::layer == Layer::Scene) {
            return;
        }

        if (!ImGui::Begin("About Sketchpad", show, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }
        ImGui::Text("Sketchpad %s", version);
        ImGui::Separator();
        ImGui::Text("Open source work by neo-mashiro, July 2021.");
        ImGui::Text("A simple framework for quickly testing out various rendering techniques in OpenGL.");
        ImGui::Separator();

        static bool show_contact_info = false;
        ImGui::Checkbox("How to reach me", &show_contact_info);

        if (show_contact_info) {
            ImGui::SameLine(0.0f, 90.0f);
            bool copy_to_clipboard = ImGui::Button("COPY", ImVec2(48.0f, 0.0f));
            ImVec2 child_size = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 2.2f);
            ImGui::BeginChildFrame(ImGui::GetID("Contact"), child_size, ImGuiWindowFlags_NoMove);

            if (copy_to_clipboard) ImGui::LogToClipboard();
            {
                ImGui::Text("Email: neo-mashiro@hotmail.com");
                ImGui::Text("Github: https://github.com/neo-mashiro");
            }
            if (copy_to_clipboard) ImGui::LogFinish();

            ImGui::EndChildFrame();
        }

        ImGui::End();
    }

    static void DrawUsageWindow(bool* show) {
        if (Window::layer == Layer::Scene) {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(Window::width / 2.9f, Window::height / 2.5f));

        if (!ImGui::Begin("How To Use", show, ImGuiWindowFlags_NoResize)) {
            ImGui::End();
            return;
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 1.0f, 0.0f, 0.2f));

        static char instructions[] =
            "This application provides a simple canvas for testing out\n"
            "various rendering techniques in computer graphics. The\n"
            "framework was built to test and learn low-level graphics\n"
            "features and how they work in modern OpenGL";

        if (ImGui::TreeNode("Basic Guide")) {
            ImGui::Spacing();
            ImGui::InputTextMultiline("##instructions", instructions, IM_ARRAYSIZE(instructions),
                ImVec2(Window::width / 3.36f, ImGui::GetTextLineHeight() * 6.0f), ImGuiInputTextFlags_ReadOnly);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mouse")) {
            ImGui::Spacing();
            ImGui::BulletText("Move the cursor around to rotate the camera.");
            ImGui::BulletText("Scroll up/down the wheels to zoom in/out the camera.");
            ImGui::Spacing();
            ImGui::TreePop();
        }

        static auto color_text = [](const char* text) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::BulletText(text);
            ImGui::PopStyleColor();
            ImGui::SameLine(128.0f, 0.0f);
        };

        if (ImGui::TreeNode("Keyboard")) {
            ImGui::Spacing();
            color_text("Enter");
            ImGui::Text("Show or hide the UI menus.");
            color_text("Escape");
            ImGui::Text("Confirm to exit the window.");
            color_text("WASD");
            ImGui::Text("Move the camera in 4 planar directions.");
            color_text("Space/Z");
            ImGui::Text("Move the camera upward/downward.");
            ImGui::Spacing();
            ImGui::TreePop();
        }

        static char menus_guide[] =
            "Each scene has a different list of menus that allow you to\n"
            "manipulate objects in the scene, for example, changing\n"
            "light intensities using a slider, or select albedo via a color\n"
            "editor. Dive into your scene of interest and play around !";

        if (ImGui::TreeNode("Menus")) {
            ImGui::Spacing();
            ImGui::InputTextMultiline("##menus_guide", menus_guide, IM_ARRAYSIZE(menus_guide),
                ImVec2(Window::width / 3.36f, ImGui::GetTextLineHeight() * 6.0f), ImGuiInputTextFlags_ReadOnly);
            ImGui::TreePop();
        }

        ImGui::PopStyleColor();
        ImGui::End();
    }

    void DrawMenuBar(const std::string& active_title, std::string& new_title) {
        static bool show_about_window, show_instructions;

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float)Window::width, 0.01f));
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 10));

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.75f));

        ImGui::Begin("Menu Bar", 0, ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Open")) {
                for (unsigned int i = 0; i < factory::titles.size(); i++) {
                    std::string title = factory::titles[i];
                    std::ostringstream id;
                    id << "  " << std::setfill('0') << std::setw(2) << i;
                    bool selected = (active_title == title);

                    if (ImGui::MenuItem((" " + title).c_str(), id.str().c_str(), selected)) {
                        if (!selected) {
                            new_title = title;
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                // bitwise or: if these windows are already open, we keep the booleans to true
                show_instructions |= ImGui::MenuItem("How To Use", "F1");
                show_about_window |= ImGui::MenuItem("About", "F8");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(3);

        if (show_instructions) {
            DrawUsageWindow(&show_instructions);
        }
        if (show_about_window) {
            DrawAboutWindow("v1.0", &show_about_window);
        }
    }

    void DrawStatusBar() {
        ImGui::SetNextWindowPos(ImVec2(0.0f, Window::height - 30.0f));
        ImGui::SetNextWindowSize(ImVec2((float)Window::width, 30.0f));
        ImGui::SetNextWindowBgAlpha(0.75f);

        ImGui::Begin("Status Bar", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::PushFont(opentype_font);

        {
            ImVec4 cyan = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
            ImVec4 red = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            ImVec4 yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            ImVec4 green = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

            ImGui::TextColored(cyan, "Cursor");
            ImGui::SameLine(0.0f, 5.0f);
            ImVec2 pos = ImGui::GetMousePos();
            ImGui::Text("(%d, %d)", (int)pos.x, (int)pos.y);
            DrawTooltip("Current mouse position in window space.");

            ImGui::SameLine(0.0f, 15.0f); DrawVerticalLine(); ImGui::SameLine(0.0f, 15.0f);

            int hours = (int)floor(Clock::time / 3600.0f);
            int minutes = (int)floor(fmod(Clock::time, 3600.0f) / 60.0f);
            int seconds = (int)floor(fmod(Clock::time, 60.0f));

            ImGui::TextColored(cyan, "Clock");
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::Text("%02d:%02d:%02d", hours, minutes, seconds);
            DrawTooltip("Time elapsed since application startup.");

            ImGui::SameLine(0.0f, 15.0f); DrawVerticalLine(); ImGui::SameLine(0.0f, 15.0f);
            ImGui::SameLine(ImGui::GetWindowWidth() - 330);

            ImGui::TextColored(cyan, "FPS");
            ImGui::SameLine(0.0f, 5.0f);
            int fps = (int)Clock::fps;
            auto text_color = fps > 200 ? green : (fps < 100 ? red : yellow);
            ImGui::TextColored(text_color, "(%d, %.2f ms)", fps, Clock::ms);
            DrawTooltip("Frames per second / milliseconds per frame.");

            ImGui::SameLine(0.0f, 15.0f); DrawVerticalLine(); ImGui::SameLine(0.0f, 15.0f);

            ImGui::TextColored(cyan, "Window");
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::Text("(%d, %d)", Window::width, Window::height);
        }

        ImGui::PopFont();
        ImGui::End();
    }

    void DrawLoadingScreen() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float win_w = (float) Window::width;
        float win_h = (float) Window::height;

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(win_w, win_h));
        ImGui::SetNextWindowBgAlpha(1.0f);

        ImGui::PushFont(opentype_font);  // font size cannot be changed after loading
        ImGui::Begin("Loading Bar", 0, ImGuiWindowFlags_NoDecoration);

        DrawRainbowBar(ImVec2(128.0f, 270.0f), 2.0f);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.3f,
            ImVec2(506, 320), ImColor(255, 255, 0, 255), "LOADING, PLEASE WAIT ......");

        float x = 345.0f, y = 385.0f;
        float size = 20.0f;
        float r, g, b;

        for (float i = 0.0f; i < 1.0f; i += 0.05f, x += size * 1.5f) {
            r = (i <= 0.33f) ? 1.0f : ((i <= 0.66f) ? 1 - (i - 0.33f) * 3 : 0.0f);
            g = (i <= 0.33f) ? i * 3 : 1.0f;
            b = (i > 0.66f) ? (i - 0.66f) * 3 : 0.0f;

            draw_list->AddTriangleFilled(ImVec2(x, y - 0.5f * size), ImVec2(x, y + 0.5f * size),
                ImVec2(x + size, y), IM_COL32(r * 255, g * 255, b * 255, 255.0f));
        }

        DrawRainbowBar(ImVec2(128.0f, y + 0.5f * size + 50.0f), 2.0f);

        ImGui::End();
        ImGui::PopFont();
    }

    void DrawRainbowBar(const ImVec2& offset, float height) {
        // draw a rainbow bar of the given height in the current window
        // the width will be automatically adjusted to center the bar in the window
        // offset is measured in pixels relative to the upper left corner of the window

        // this function is borrowed and modified from the unknown cheats forum
        // source: https://www.unknowncheats.me/forum/2550901-post1.html

        float speed = 0.0006f;
        static float static_hue = 0.0f;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos() + offset;
        float width = ImGui::GetWindowWidth() - offset.x * 2.0f;

        static_hue -= speed;
        if (static_hue < -1.0f) {
            static_hue += 1.0f;
        }

        for (int i = 0; i < width; i++) {
            float hue = static_hue + (1.0f / width) * i;
            if (hue < 0.0f) hue += 1.0f;
            ImColor color = ImColor::HSV(hue, 1.0f, 1.0f);
            draw_list->AddRectFilled(ImVec2(pos.x + i, pos.y), ImVec2(pos.x + i + 1, pos.y + height), color);
        }
    }

}
