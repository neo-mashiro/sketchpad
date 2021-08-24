#pragma once

#include <string>

#define IMGUI_DISABLE_METRICS_WINDOW
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "components/all.h"
#include "scene/entity.h"

namespace ui {

    extern ImFont* truetype_font;
    extern ImFont* opentype_font;

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
}
