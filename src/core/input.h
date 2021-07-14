#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <unordered_map>

namespace core {

    enum class Axis : uint8_t { X = 1, Y = 2 };

    class Input {
      private:
        static inline GLuint mouse_pos_x = 1280 / 2;
        static inline GLuint mouse_pos_y = 720 / 2;
        static inline int mouse_delta_x = 0;
        static inline int mouse_delta_y = 0;
        static inline float mouse_zoom = 0.0f;

      public:
        static bool IsKeyPressed(unsigned char key);
        static bool IsScrollWheelDown(int button, int state);
        static bool IsScrollWheelUp(int button, int state);

        static GLuint ReadMouseAxis(Axis axis);
        static float ReadMouseZoom(void);

        static void SetKeyState(unsigned char key, bool pressed);
        static void SetMouseMove(int new_x, int new_y);
        static void SetMouseZoom(float delta_zoom);
    };
}
