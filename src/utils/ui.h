#pragma once

#include <string>

#define IMGUI_DISABLE_METRICS_WINDOW
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_internal.h"

namespace ui {

    extern ImFont* truetype_font;
    extern ImFont* opentype_font;

    void Init(void);
    void Clear(void);

    void NewFrame(void);
    void EndFrame(void);

    void DrawVerticalLine(void);
    void DrawTooltip(const char* desc, float spacing = 5.0f);

    void DrawMenuBar(const std::string& active_title, std::string& new_title);
    void DrawStatusBar(void);
    void DrawLoadingScreen(void);

    void DrawRainbowBar(const ImVec2& offset, float height);
}
