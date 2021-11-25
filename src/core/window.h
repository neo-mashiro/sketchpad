#pragma once

#include <string>
#include <glad/glad.h>

struct GLFWwindow;  // forward declaration

namespace core {

    enum class Layer : uint8_t {
        Scene,
        ImGui,
        Win32
    };

    class Window {
      public:
        static inline std::string title = "sketchpad";
        static inline GLuint width = 0, height = 0;
        static inline GLuint pos_x = 0, pos_y = 0;
        static inline float aspect_ratio = 16.0f / 9.0f;

        static inline unsigned int window_id = 0;        // GLUT window handle
        static inline GLFWwindow* window_ptr = nullptr;  // GLFW window handle

        static inline Layer layer = Layer::Scene;

      public:
        static void Init();
        static void Clear();

        static void Rename(const std::string& new_title);
        static void Resize();

        static void OnLayerSwitch();
        static void OnScreenshots();
        static void OnOpenBrowser();
        static bool OnExitRequest();
    };

}
