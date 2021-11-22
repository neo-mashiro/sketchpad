#pragma once

#include <string>

#define IMGUI_DISABLE_METRICS_WINDOW
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <IconsForkAwesome.h>

#include "components/all.h"
#include "scene/entity.h"

namespace scene::ui {

    extern ImFont* truetype_font;  // TrueType, Lato-Regular, 18pt (main font)
    extern ImFont* opentype_font;  // OpenType, Palatino Linotype, 17pt (sub font)
    extern ImFont* web_icon_font;  // Fork Awesome web icon font, 18pt

    void Init(void);
    void Clear(void);

    void NewFrame(void);
    void EndFrame(void);

    // window-level helper functions, use them to facilitate drawing in your own scene
    void LoadInspectorConfig(void);
    void DrawVerticalLine(void);
    void DrawTooltip(const char* desc, float spacing = 5.0f);
    void DrawRainbowBar(const ImVec2& offset, float height);
    void DrawGizmo(scene::Entity camera, scene::Entity target);

    // application-level drawing functions, used by the core module, don't touch
    void DrawMenuBar(const std::string& active_title, std::string& new_title);
    void DrawStatusBar(void);
    void DrawWelcomeScreen(ImTextureID id);
    void DrawLoadingScreen(void);
    void DrawCrosshair(void);
}
