#pragma once

#include <unordered_map>
#include <GL/glew.h>
#include <GL/freeglut.h>

namespace core {

    enum class Axis : uint8_t {
        Horizontal,
        Vertical
    };

    class Input {
      private:
        static std::unordered_map<uint8_t, bool> keybook;
        static inline int mouse_pos_x = 1280 / 2;
        static inline int mouse_pos_y = 720 / 2;
        static inline int mouse_delta_x = 0;
        static inline int mouse_delta_y = 0;
        static inline float mouse_zoom = 0.0f;

      public:
        static bool IsKeyPressed(unsigned char key);
        static bool IsScrollWheelDown(int button, int state);
        static bool IsScrollWheelUp(int button, int state);
        
        static int GetMouseAxis(Axis axis);
        static float GetMouseZoom(void);

        static void SetKeyState(unsigned char key, bool pressed);
        static void SetMouseMove(int new_x, int new_y);
        static void SetMouseZoom(float delta_zoom);

        static void ShowCursor(void);
        static void HideCursor(void);
        static void ResetCursor(void);
    };
}
