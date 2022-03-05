#include "pch.h"

#define IMGUI_DISABLE_METRICS_WINDOW
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_glut.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imstb_rectpack.h>
#include <imgui/imstb_textedit.h>
#include <imgui/imstb_truetype.h>
#include <ImGuizmo/ImGuizmo.h>
#include <IconsForkAwesome.h>

#include "core/base.h"
#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "component/all.h"
#include "scene/entity.h"
#include "scene/factory.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/path.h"

using namespace core;
using namespace component;
using namespace ImGui;

namespace scene::ui {

    ImFont* truetype_font;  // TrueType, Lato-Regular, 18pt (main font)
    ImFont* opentype_font;  // OpenType, Palatino Linotype, 17pt (sub font)
    ImFont* web_icon_font;  // Fork Awesome web icon font, 18pt

    // private global variables
    static ImVec2 window_center;
    static ImVec4 red    = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    static ImVec4 yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    static ImVec4 green  = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    static ImVec4 blue   = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    static ImVec4 cyan   = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);

    static ImGuiWindowFlags invisible_window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;
    static int rotation_start_index = 0;

    void Init() {
        CreateContext();
        ImGuiIO& io = GetIO();
        ImGuiStyle& style = GetStyle();

        window_center = ImVec2((float)Window::width, (float)Window::height) * 0.5f;

        // load fonts from the resource folder
        float fontsize_main = 18.0f;
        float fontsize_icon = 18.0f;  // bake icon font into the main font
        float fontsize_sub  = 17.0f;

        std::string ttf_main = utils::paths::font + "Lato.ttf";
        std::string ttf_sub  = utils::paths::font + "palatino.ttf";
        std::string ttf_icon = utils::paths::font + FONT_ICON_FILE_NAME_FK;

        ImFontConfig config_main;
        config_main.PixelSnapH = true;
        config_main.OversampleH = 4;
        config_main.OversampleV = 4;
        config_main.RasterizerMultiply = 1.2f;  // brighten up the font to make them more readable
        config_main.GlyphExtraSpacing.x = 0.0f;

        ImFontConfig config_sub;
        config_sub.PixelSnapH = true;
        config_sub.OversampleH = 4;
        config_sub.OversampleV = 4;
        config_sub.RasterizerMultiply = 1.25f;  // brighten up the font to make them more readable
        config_sub.GlyphExtraSpacing.x = 0.0f;

        ImFontConfig config_icon;
        config_icon.MergeMode = true;
        config_icon.PixelSnapH = true;
        config_icon.OversampleH = 4;
        config_icon.OversampleV = 4;
        config_icon.RasterizerMultiply = 1.5f;  // brighten up the font to make them more readable
        config_icon.GlyphOffset.y = 0.0f;       // tweak this to vertically align with the main font
        config_icon.GlyphMinAdvanceX = fontsize_main;  // enforce monospaced icon font
        config_icon.GlyphMaxAdvanceX = fontsize_main;  // enforce monospaced icon font

        static const ImWchar icon_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };  // zero-terminated
        
        truetype_font = io.Fonts->AddFontFromFileTTF(ttf_main.c_str(), fontsize_main, &config_main);
        web_icon_font = io.Fonts->AddFontFromFileTTF(ttf_icon.c_str(), fontsize_icon, &config_icon, icon_ranges);
        opentype_font = io.Fonts->AddFontFromFileTTF(ttf_sub.c_str(), fontsize_sub, &config_sub);

        // build font textures
        unsigned char* pixels;
        int width, height, bytes_per_pixel;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

        // load default dark theme
        StyleColorsDark();

        // setup custom styles
        style.WindowBorderSize         = 0.0f;
        style.FrameBorderSize          = 1.0f;
        style.PopupBorderSize          = 1.0f;
        style.ChildBorderSize          = 1.0f;
        style.TabBorderSize            = 0.0f;
        style.ScrollbarSize            = 18.0f;
        style.GrabMinSize              = 10.0f;

        style.WindowPadding            = ImVec2(8.0f, 8.0f);
        style.FramePadding             = ImVec2(4.0f, 6.0f);
        style.ItemSpacing              = ImVec2(10.0f, 10.0f);
        style.ItemInnerSpacing         = ImVec2(10.0f, 10.0f);
        style.IndentSpacing            = 16.0f;

        style.WindowRounding           = 0.0f;
        style.ChildRounding            = 0.0f;
        style.FrameRounding            = 4.0f;
        style.PopupRounding            = 0.0f;
        style.TabRounding              = 4.0f;
        style.GrabRounding             = 4.0f;
        style.ScrollbarRounding        = 12.0f;

        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ColorButtonPosition      = ImGuiDir_Right;

        style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
        style.WindowTitleAlign         = ImVec2(0.0f, 0.5f);
        style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

        style.AntiAliasedLines         = true;
        style.AntiAliasedFill          = true;
        style.AntiAliasedLinesUseTex   = true;

        // setup custom colors
        auto& c = GetStyle().Colors;

        c[ImGuiCol_WindowBg]             = ImVec4(0.0f, 0.0f, 0.0f, 0.85f);
        c[ImGuiCol_ChildBg]              = ImVec4(0.1f, 0.1f, 0.1f, 0.85f);
        c[ImGuiCol_PopupBg]              = ImVec4(0.1f, 0.1f, 0.1f, 0.85f);

        c[ImGuiCol_FrameBg]              = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);
        c[ImGuiCol_FrameBgHovered]       = ImVec4(0.2f, 0.2f, 0.2f, 0.75f);
        c[ImGuiCol_FrameBgActive]        = ImVec4(0.3f, 0.3f, 0.3f, 0.75f);

        c[ImGuiCol_TitleBg]              = ImVec4(0.2f, 0.2f, 0.2f, 0.75f);
        c[ImGuiCol_TitleBgActive]        = ImVec4(0.0f, 0.3f, 0.0f, 0.9f);
        c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);

        c[ImGuiCol_ScrollbarBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);
        c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.9f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.4f, 0.4f, 0.4f, 0.9f);

        c[ImGuiCol_CheckMark]            = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        c[ImGuiCol_SliderGrab]           = ImVec4(0.0f, 0.4f, 0.0f, 0.9f);
        c[ImGuiCol_SliderGrabActive]     = ImVec4(0.0f, 0.5f, 0.0f, 0.9f);

        c[ImGuiCol_Button]               = ImVec4(0.0f, 0.3f, 0.0f, 0.9f);
        c[ImGuiCol_ButtonHovered]        = ImVec4(0.0f, 0.55f, 0.0f, 0.9f);
        c[ImGuiCol_ButtonActive]         = ImVec4(0.0f, 0.5f, 0.0f, 0.9f);

        c[ImGuiCol_Header]               = ImVec4(0.5f, 0.0f, 1.0f, 0.5f);
        c[ImGuiCol_HeaderHovered]        = ImVec4(0.5f, 0.0f, 1.0f, 0.8f);
        c[ImGuiCol_HeaderActive]         = ImVec4(0.5f, 0.0f, 1.0f, 0.7f);

        c[ImGuiCol_Tab]                  = ImVec4(0.0f, 0.3f, 0.0f, 0.8f);
        c[ImGuiCol_TabHovered]           = ImVec4(0.0f, 0.4f, 0.0f, 0.8f);
        c[ImGuiCol_TabActive]            = ImVec4(0.0f, 0.4f, 0.0f, 0.8f);
        c[ImGuiCol_TabUnfocused]         = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);
        c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);

        if constexpr (_freeglut) {
            ImGui_ImplGLUT_Init();
            ImGui_ImplOpenGL3_Init();
        }
        else {
            ImGui_ImplGlfw_InitForOpenGL(Window::window_ptr, false);
            ImGui_ImplOpenGL3_Init();
        }
    }

    void Clear() {
        if constexpr (_freeglut) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGLUT_Shutdown();
        }
        else {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
        }

        DestroyContext();
    }

    void NewFrame() {
        if constexpr (_freeglut) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGLUT_NewFrame();
            ImGuizmo::BeginFrame();
        }
        else {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();  // for GLFW backend we need to call this manually
            ImGuizmo::BeginFrame();
        }
    }

    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
    }

    bool NewInspector(void) {
        static const float w = 256.0f * 1.25f;  // tweaked for 1600 x 900 resolution
        static const float h = 612.0f * 1.25f;

        SetNextWindowPos(ImVec2(Window::width - w, (Window::height - h) * 0.5f));
        SetNextWindowSize(ImVec2(w, h));

        static ImGuiWindowFlags inspector_flags = ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        PushID("Inspector Window");

        if (Begin(ICON_FK_LOCATION_ARROW " Inspector", 0, inspector_flags)) {
            return true;
        }

        CORE_ERROR("Failed to load inspector due to clipping issues...");
        CORE_ERROR("Did you draw a full screen opaque window?");
        return false;
    }

    void EndInspector(void) {
        End();
        PopID();
    }

    void DrawVerticalLine() {
        SeparatorEx(ImGuiSeparatorFlags_Vertical);
    }

    void DrawTooltip(const char* desc, float spacing) {
        SameLine(0.0f, spacing);
        TextDisabled("(?)");

        if (IsItemHovered()) {
            PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            BeginTooltip();
            PushTextWrapPos(GetFontSize() * 35.0f);
            TextUnformatted(desc);
            PopTextWrapPos();
            EndTooltip();
            PopStyleColor(2);
        }
    }

    void DrawRainbowBar(const ImVec2& offset, float height) {
        // draw a rainbow bar of the given height in the current window
        // bar width will be automatic adjusted to center at the window
        // offset is in pixels relative to the window's upper left corner

        // this function is borrowed and modified from the unknown cheats forum
        // source: https://www.unknowncheats.me/forum/2550901-post1.html

        float speed = 0.0006f;
        static float static_hue = 0.0f;

        ImDrawList* draw_list = GetWindowDrawList();
        ImVec2 pos = GetWindowPos() + offset;
        float width = GetWindowWidth() - offset.x * 2.0f;

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

    void DrawGizmo(Entity& camera, Entity& target, Gizmo z) {
        static const ImVec2 win_pos = ImVec2(0.0f, 50.0f);
        static const ImVec2 win_size = ImVec2((float)Window::width, (float)Window::height - 82.0f);

        ImGuizmo::MODE mode = ImGuizmo::MODE::LOCAL;
        ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;

        switch (z) {
            case Gizmo::Translate: {
                operation = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Gizmo::Rotate: {
                operation = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Gizmo::Scale: {
                operation = ImGuizmo::OPERATION::SCALE;
                break;
            }
            case Gizmo::Bounds: case Gizmo::None: default: {
                return;
            }
        }

        auto& T = target.GetComponent<Transform>();
        auto& C = camera.GetComponent<Camera>();
        glm::mat4 V = C.GetViewMatrix();
        glm::mat4 P = C.GetProjectionMatrix();

        // convert model matrix to left-handed as ImGuizmo assumes a left-handed coordinate system
        static const glm::vec3 RvL = glm::vec3(1.0f, 1.0f, -1.0f);  // scaling vec for R2L and L2R
        glm::mat4 transform = glm::scale(T.transform, RvL);

        SetNextWindowPos(win_pos);    // below the menu bar
        SetNextWindowSize(win_size);  // above the status bar
        Begin("##Invisible Gizmo Window", 0, invisible_window_flags);

        ImGuizmo::SetOrthographic(true);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(win_pos.x, win_pos.y, win_size.x, win_size.y);
        ImGuizmo::Manipulate(value_ptr(V), value_ptr(P), operation, mode, value_ptr(transform));

        // if the gizmo is being manipulated, which means that the transform matrix may have been
        // (but not necessarily) modified by the gizmo from UI, we update the transform component.
        // note that guizmos are supposed to be drawn and manipulated one at a time, if multiple
        // gizmos for multiple objects are rendered at the same time, trying to manipulate one of
        // them will affect all the others because `ImGuizmo::Manipulate` will not work properly.
        // in addition `ImGuizmo::IsUsing` can't tell which specific object is being used, it has
        // only one global context that applies to all objects whose guizmo is currently rendered

        if (ImGuizmo::IsUsing()) {
            transform = glm::scale(transform, RvL);  // convert back to right-handed
            T.SetTransform(transform);
        }

        ImGui::End();
    }

    void PushRotation() {
        rotation_start_index = GetWindowDrawList()->VtxBuffer.Size;
    }

    void PopRotation(float radians, bool ccw) {
        // vertex buffer lower and upper bounds (p_min, p_max)
        static const float max_float = std::numeric_limits<float>::max();
        ImVec2 lower(+max_float, +max_float);
        ImVec2 upper(-max_float, -max_float);

        auto& buff = GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buff.Size; ++i) {
            lower = ImMin(lower, buff[i].pos);
            upper = ImMax(upper, buff[i].pos);
        }

        // use the buffer center as rotation's pivot point
        ImVec2 center = ImVec2((lower.x + upper.x) / 2, (lower.y + upper.y) / 2);

        float s = sin(radians);
        float c = cos(radians);
        
        for (int i = rotation_start_index; i < buff.Size; ++i) {
            ImVec2 offset = buff[i].pos - center;  // vertex offset relative to the center pivot
            ImVec2 offset_new = ccw ? ImRotate(offset, s, c) : ImRotate(offset, c, s);  // rotate about center
            buff[i].pos = center + offset_new;  // update rotated vertex position (center stays put)
        }
    }

    int DrawPopupModal(const char* title, const char* message, const ImVec2& size) {
        int button_id = -1;  // none of the buttons was pressed
        OpenPopup(title);

        ImVec2 center = GetMainViewport()->GetCenter();
        SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        SetNextWindowSize(size);

        PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 8.0f));

        float button_indentation = size.x * 0.1f;
        float text_width = CalcTextSize(message).x;
        float text_indentation = (size.x - text_width) * 0.5f;  // center the text locally to the window

        // if text too long to be drawn on one line, enforce a minimum indentation
        text_indentation = std::max(text_indentation, 20.0f);

        if (BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            SameLine(text_indentation);
            PushTextWrapPos(size.x - text_indentation);
            TextWrapped(message);
            PopTextWrapPos();
            Separator();

            static const ImVec2 vspacing = ImVec2(0.0f, 0.1f);
            Dummy(vspacing);

            if (Indent(button_indentation); true) {
                if (Button("OK", ImVec2(100.0f, 0.0f))) {
                    CloseCurrentPopup();
                    button_id = 0;  // OK
                }

                SetItemDefaultFocus();
                SameLine(0.0f, size.x - 2.0f * (100.0f + button_indentation));

                if (Button("Cancel", ImVec2(100.0f, 0.0f))) {
                    CloseCurrentPopup();
                    button_id = 1;  // cancel
                }
            }

            Unindent(button_indentation);
            Dummy(vspacing);
            EndPopup();
        }

        PopStyleVar(2);
        return button_id;
    }

    glm::ivec2 GetCursorPosition() {
        if (Window::layer == Layer::Scene) {
            return Input::GetCursorPosition();
        }

        auto mouse_pos = ImGui::GetMousePos();
        return glm::ivec2(mouse_pos.x, mouse_pos.y);
    }

    static void DrawAboutWindow(const char* version, bool* show) {
        if (Window::layer == Layer::Scene) {
            return;
        }

        if (!Begin("About Sketchpad", show, ImGuiWindowFlags_AlwaysAutoResize)) {
            End();
            return;
        }

        Text("Sketchpad %s", version);
        Separator();
        Text("Open source work by neo-mashiro, July 2021.");
        Text("A simple OpenGL sandbox renderer for experimenting with various rendering techniques.");
        Separator();

        static bool show_contact_info = false;
        Checkbox("How to reach me", &show_contact_info);

        if (show_contact_info) {
            SameLine(0.0f, 90.0f);
            bool copy_to_clipboard = Button("COPY", ImVec2(48.0f, 0.0f));
            ImVec2 child_size = ImVec2(0, GetTextLineHeightWithSpacing() * 2.2f);
            BeginChildFrame(GetID("Contact"), child_size, ImGuiWindowFlags_NoMove);

            if (copy_to_clipboard) LogToClipboard();
            {
                Text("Email: neo-mashiro@hotmail.com");
                Text("Github: https://github.com/neo-mashiro");
            }
            if (copy_to_clipboard) LogFinish();

            EndChildFrame();
        }

        End();
    }

    static void DrawUsageWindow(bool* show) {
        if (Window::layer == Layer::Scene) {
            return;
        }

        SetNextWindowSize(ImVec2(1280.0f / 2.82f, 720.0f / 1.6f));

        if (!Begin("How To Use", show, ImGuiWindowFlags_NoResize)) {
            End();
            return;
        }

        Spacing();

        static const ImVec4 text_color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        static const char instructions[] =
            "This software is a simple sandbox for playing with modern graphics rendering in OpenGL. "
            "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog";

        if (TreeNode("Basic Guide")) {
            Spacing();
            Indent(10.0f);
            PushStyleColor(ImGuiCol_Text, text_color);
            PushTextWrapPos(412.903f);
            TextWrapped(instructions);
            PopTextWrapPos();
            PopStyleColor();
            Unindent(10.0f);
            TreePop();
        }

        if (TreeNode("Mouse")) {
            Spacing();
            BulletText("Move the cursor around to rotate the camera.");
            BulletText("Hold the right button and slide to zoom in & out.");
            Spacing();
            TreePop();
        }

        static auto color_text = [](const char* text) {
            PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            BulletText(text);
            PopStyleColor();
            SameLine(128.0f, 0.0f);
        };

        if (TreeNode("Keyboard")) {
            Spacing();
            color_text("Enter");
            Text("Show or hide the UI menus.");
            color_text("Escape");
            Text("Confirm to exit the window.");
            color_text("WASD");
            Text("Move the camera in 4 planar directions.");
            color_text("Space/Z");
            Text("Move the camera upward/downward.");
            color_text("R");
            Text("Recover camera to the initial setup.");
            Spacing();
            TreePop();
        }

        static const char menus_guide[] =
            "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. "
            "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog.";

        if (TreeNode("Menus")) {
            Spacing();
            Indent(10.0f);
            PushStyleColor(ImGuiCol_Text, text_color);
            PushTextWrapPos(412.903f);
            TextWrapped(menus_guide);
            PopTextWrapPos();
            PopStyleColor();
            Unindent(10.0f);
            TreePop();
        }

        End();
    }

    void DrawMenuBar(std::string& new_title) {
        static bool show_about_window = false;
        static bool show_instructions = false;
        static bool show_home_popup = false;
        static bool music_on = true;

        auto& curr_scene_title = Renderer::GetScene()->title;

        SetNextWindowPos(ImVec2(0.0f, 0.0f));
        SetNextWindowSize(ImVec2((float)Window::width, 0.01f));
        SetNextWindowBgAlpha(0.0f);

        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 10));
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 10));

        PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
        PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.55f));
        PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.39f, 0.61f, 0.8f));
        PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.24f, 0.54f, 0.89f, 0.8f));

        Begin("Menu Bar", 0, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        if (BeginMenuBar()) {
            PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.7f, 0.7f, 0.3f));
            PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));

            if (BeginMenu("Open")) {
                for (unsigned int i = 0; i < scene::factory::titles.size(); i++) {
                    std::string title = scene::factory::titles[i];
                    std::ostringstream id;
                    id << " " << std::setfill('0') << std::setw(2) << i;
                    bool selected = (curr_scene_title == title);

                    if (MenuItem((" " + title).c_str(), id.str().c_str(), selected)) {
                        if (!selected) {
                            new_title = title;
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (BeginMenu("Options")) {
                // currently we only support a fixed resolution at 1600 x 900, this menu is simply
                // a dummy functionality, other resolutions are not yet supported so clicking on
                // them would have no effect. On desktop monitors, it would be more comfortable to
                // adopt a full HD or QHD resolution instead, but we need to recalculate the pixel
                // offsets of some UI panels to adapt the change. The real challenge of supporting
                // multiple resolutions is that we also need to adjust the size of our framebuffer
                // textures on the fly, which is expensive and takes quite a bit of work. If you'd
                // like to do so, you should consider applying this function to the welcome screen
                // exclusively, as we don't want to break the data state in the middle of a scene.

                if (BeginMenu(" Window Resolution")) {
                    MenuItem(" 1280 x 720",  NULL, false);
                    MenuItem(" 1600 x 900",  NULL, true);   // active resolution
                    MenuItem(" 1920 x 1080", NULL, false);  // Full HD
                    MenuItem(" 2560 x 1440", NULL, false);  // QHD
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (BeginMenu("Help")) {
                // bitwise or: if these windows are already open, we keep the booleans to true
                show_instructions |= MenuItem(" How To Use", "F1");
                show_about_window |= MenuItem(" About", "F8");
                ImGui::EndMenu();
            }

            // menu icon items
            SameLine(GetWindowWidth() - 303.0f);
            static const ImVec4 tooltip_bg_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

            if (MenuItem(ICON_FK_HOME)) {
                if (curr_scene_title != "Welcome Screen") {
                    show_home_popup = true;
                }
            }
            else if (IsItemHovered()) {
                PushStyleColor(ImGuiCol_PopupBg, tooltip_bg_color);
                BeginTooltip();
                TextUnformatted("Back to main menu");
                EndTooltip();
                PopStyleColor();
            }

            if (MenuItem(Window::layer == Layer::ImGui ? ICON_FK_PICTURE_O : ICON_FK_COFFEE)) {
                Input::SetKeyDown(VK_RETURN, true);
            }
            else if (IsItemHovered()) {
                PushStyleColor(ImGuiCol_PopupBg, tooltip_bg_color);
                BeginTooltip();
                TextUnformatted("Back to scene mode (Enter)");
                EndTooltip();
                PopStyleColor();
            }

            if (MenuItem(music_on ? ICON_FK_VOLUME_UP : ICON_FK_VOLUME_MUTE)) {
                music_on = !music_on;
            }
            else if (IsItemHovered()) {
                PushStyleColor(ImGuiCol_PopupBg, tooltip_bg_color);
                BeginTooltip();
                TextUnformatted("Music On/Off");
                EndTooltip();
                PopStyleColor();
            }

            if (MenuItem(ICON_FK_CAMERA)) {
                Window::OnScreenshots();
            }
            else if (IsItemHovered()) {
                PushStyleColor(ImGuiCol_PopupBg, tooltip_bg_color);
                BeginTooltip();
                TextUnformatted("Take a screenshot");
                EndTooltip();
                PopStyleColor();
            }

            if (MenuItem(ICON_FK_EXTERNAL_LINK)) {
                Window::OnOpenBrowser();
            }
            else if (IsItemHovered()) {
                PushStyleColor(ImGuiCol_PopupBg, tooltip_bg_color);
                BeginTooltip();
                TextUnformatted("Go to website");
                EndTooltip();
                PopStyleColor();
            }

            if (MenuItem(ICON_FK_COG)) {
                // TODO: open profiler window
            }
            else if (IsItemHovered()) {
                PushStyleColor(ImGuiCol_PopupBg, tooltip_bg_color);
                BeginTooltip();
                TextUnformatted("Profiler window");
                EndTooltip();
                PopStyleColor();
            }

            if (MenuItem(ICON_FK_POWER_OFF)) {
                Input::SetKeyDown(VK_ESCAPE, true);
            }
            else if (IsItemHovered()) {
                PushStyleColor(ImGuiCol_PopupBg, tooltip_bg_color);
                BeginTooltip();
                TextUnformatted("Close (Esc)");
                EndTooltip();
                PopStyleColor();
            }

            PopStyleColor(2);
            EndMenuBar();
        }

        End();

        PopStyleColor(4);
        PopStyleVar(3);

        if (show_instructions) {
            DrawUsageWindow(&show_instructions);
        }

        if (show_about_window) {
            DrawAboutWindow("v1.0", &show_about_window);
        }

        if (show_home_popup) {
            static const char* message = "\nDo you want to return to the main menu?\n\n";
            static const ImVec2 popup_size = ImVec2(360.0f, 172.0f);
            int button_id = DrawPopupModal(curr_scene_title.c_str(), message, popup_size);
            if (button_id == 0) {  // OK
                show_home_popup = false;
                new_title = "Welcome Screen";
            }
            else if (button_id == 1) {  // cancel
                show_home_popup = false;
            }
        }
    }

    void DrawStatusBar() {
        SetNextWindowPos(ImVec2(0.0f, Window::height - 32.0f));
        SetNextWindowSize(ImVec2((float)Window::width, 32.0f));
        SetNextWindowBgAlpha(0.75f);

        Begin("Status Bar", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        PushFont(opentype_font);

        {
            SameLine(0.0f, 9.0f);
            TextColored(cyan, "Cursor");
            SameLine(0.0f, 5.0f);
            ImVec2 pos = Window::layer == Layer::ImGui ? GetMousePos() : window_center;
            Text("(%d, %d)", (int)pos.x, (int)pos.y);
            DrawTooltip("Current mouse position in window space.");

            SameLine(0.0f, 15.0f); DrawVerticalLine(); SameLine(0.0f, 15.0f);

            int hours = (int)floor(Clock::time / 3600.0f);
            int minutes = (int)floor(fmod(Clock::time, 3600.0f) / 60.0f);
            int seconds = (int)floor(fmod(Clock::time, 60.0f));

            TextColored(cyan, "Clock");
            SameLine(0.0f, 5.0f);
            Text("%02d:%02d:%02d", hours, minutes, seconds);
            DrawTooltip("Time elapsed since application startup.");

            SameLine(0.0f, 15.0f); DrawVerticalLine(); SameLine(0.0f, 15.0f);
            SameLine(GetWindowWidth() - 355);

            TextColored(cyan, "FPS");
            SameLine(0.0f, 5.0f);
            int fps = (int)Clock::fps;
            auto text_color = fps > 90 ? green : (fps < 30 ? red : yellow);
            TextColored(text_color, "(%d, %.2f ms)", fps, Clock::ms);
            DrawTooltip("Frames per second / milliseconds per frame.");

            SameLine(0.0f, 15.0f); DrawVerticalLine(); SameLine(0.0f, 15.0f);

            TextColored(cyan, "Window");
            SameLine(0.0f, 5.0f);
            Text("(%d, %d)", Window::width, Window::height);
        }

        PopFont();
        End();
    }

    void DrawWelcomeScreen(ImTextureID id) {
        ImDrawList* draw_list = GetBackgroundDrawList();
        const static float win_w = (float)Window::width;
        const static float win_h = (float)Window::height;
        draw_list->AddImage(id, ImVec2(0.0f, 0.0f), ImVec2(win_w, win_h));
    }

    void DrawLoadingScreen() {
        const static float win_w = (float) Window::width;
        const static float win_h = (float) Window::height;
        const static float bar_w = 268.0f;
        const static float bar_h = 80.0f;

        scene::Renderer::Clear();

        SetNextWindowPos(ImVec2(0.0f, 0.0f));
        SetNextWindowSize(ImVec2(win_w, win_h));
        SetNextWindowBgAlpha(1.0f);

        PushFont(opentype_font);
        Begin("Loading Bar", 0, ImGuiWindowFlags_NoDecoration);

        ImDrawList* draw_list = GetWindowDrawList();
        draw_list->AddText(GetFont(), GetFontSize() * 1.3f, ImVec2(win_w - bar_w, win_h - bar_h) * 0.5f,
            ColorConvertFloat4ToU32(yellow), "LOADING, PLEASE WAIT ......");

        float x = 505.0f;  // not magic number, it's simple math!
        float y = 465.0f;  // not magic number, it's simple math!
        const static float size = 20.0f;
        float r, g, b;

        for (float i = 0.0f; i < 1.0f; i += 0.05f, x += size * 1.5f) {
            r = (i <= 0.33f) ? 1.0f : ((i <= 0.66f) ? 1 - (i - 0.33f) * 3 : 0.0f);
            g = (i <= 0.33f) ? i * 3 : 1.0f;
            b = (i > 0.66f) ? (i - 0.66f) * 3 : 0.0f;

            draw_list->AddTriangleFilled(ImVec2(x, y - 0.5f * size), ImVec2(x, y + 0.5f * size),
                ImVec2(x + size, y), IM_COL32(r * 255, g * 255, b * 255, 255.0f));
        }

        End();
        PopFont();
    }

    void DrawCrosshair() {
        ImDrawList* draw_list = GetForegroundDrawList();

        static ImU32 color = IM_COL32(0, 255, 0, 255);
        static ImVec2 lines[4][2] = {
            { window_center + ImVec2(+3.0f, +0.0f), window_center + ImVec2(+9.0f, +0.0f) },
            { window_center + ImVec2(+0.0f, +3.0f), window_center + ImVec2(+0.0f, +9.0f) },
            { window_center + ImVec2(-3.0f, +0.0f), window_center + ImVec2(-9.0f, +0.0f) },
            { window_center + ImVec2(+0.0f, -3.0f), window_center + ImVec2(+0.0f, -9.0f) }
        };

        draw_list->AddLine(lines[0][0], lines[0][1], color);
        draw_list->AddLine(lines[1][0], lines[1][1], color);
        draw_list->AddLine(lines[2][0], lines[2][1], color);
        draw_list->AddLine(lines[3][0], lines[3][1], color);
    }

}
