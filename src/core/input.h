#pragma once

#include <unordered_map>
#include <glm/glm.hpp>

namespace core {

    enum class MouseAxis : char {
        Horizontal, Vertical
    };

    enum class MouseButton : char {
        Left, Middle, Right,
    };

    class Input {
      private:
        // keystroke states: pressed or released
        static std::unordered_map<uint8_t, bool> keybook;

        // cursor position and offsets along each axis
        static inline float cursor_pos_x = 0;
        static inline float cursor_pos_y = 0;
        static inline float cursor_delta_x = 0;
        static inline float cursor_delta_y = 0;

        // mouse button states: pressed or released
        static inline bool mouse_button_l = false;
        static inline bool mouse_button_r = false;
        static inline bool mouse_button_m = false;

        // scrollwheel's vertical offset: > 0, < 0 or = 0
        static inline float scroll_offset = 0.0f;

      public:
        static void Clear();

        static void ShowCursor();
        static void HideCursor();

        static void SetKeyDown(unsigned char key, bool pressed);
        static bool GetKeyDown(unsigned char key);

        static void SetMouseDown(MouseButton button, bool pressed);
        static bool GetMouseDown(MouseButton button);

        static void SetCursor(float new_x, float new_y);
        static float GetCursorOffset(MouseAxis axis);
        static float GetCursorPosition(MouseAxis axis);
        static glm::ivec2 GetCursorPosition();

        static void SetScroll(float offset);
        static float GetScrollOffset(void);
    };

}
