#pragma once

#include <string>
#include <GL/glew.h>
#include <GL/freeglut.h>

namespace core {

    enum class Layer { Scene, ImGui, Win32 };

    class Window {
      public:
        static inline std::string title = "sketchpad";
        static inline int id = -1;

        static inline GLuint width = 1280, height = 720;
        static inline GLuint pos_x = 0, pos_y = 0;
        static inline GLuint display_mode = 0;
        static inline float aspect_ratio = 1280.0f / 720.0f;
        static inline Layer layer = Layer::Scene;

        static void Init();
        static void Clear();

        static void Rename(const std::string& new_title);
        static void Reshape();
        static void Refresh();

        static void ConfirmExit();
        static void ToggleImGui();
    };
}
