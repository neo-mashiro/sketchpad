#pragma once

#include <string>
#include <GL/glew.h>
#include <GL/freeglut.h>

namespace core {

    enum class Layer {
        Scene,
        ImGui,
        Win32
    };

    enum class Resolution {
        Normal,  // 1280 x 720
        Large    // 1600 x 900
    };

    class Window {
      public:
        static inline std::string title = "sketchpad";
        static inline int id = -1;

        static inline Layer layer = Layer::Scene;
        static inline Resolution resolution = Resolution::Normal;

        static inline GLuint width = 0, height = 0;
        static inline GLuint pos_x = 0, pos_y = 0;
        static inline GLuint display_mode = 0;
        static inline float aspect_ratio = 16.0f / 9.0f;

        static void Init(Resolution res);
        static void Clear();

        static void Rename(const std::string& new_title);
        static void Resize(Resolution res);
        static void Reshape();
        static void Refresh();

        static void ConfirmExit();
        static void ToggleImGui();
    };
}
