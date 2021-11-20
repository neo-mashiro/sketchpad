#include "pch.h"

#include <GL/freeglut.h>
#include <GLFW/glfw3.h>
#include "core/input.h"
#include "core/window.h"

namespace core {

    #pragma warning(push)
    #pragma warning(disable : 4244)

    // internally, all keys are defined by standard Win32 virtual-key codes and ASCII codes
    // if an input library defines its own key codes, they must be remapped to our standard
    std::unordered_map<unsigned char, bool> Input::keybook {
        { 'w', 0 }, { 's', 0 }, { 'a', 0 }, { 'd', 0 }, { 'z', 0 },  // WASD + Z
        { VK_SPACE, 0 }, { VK_RETURN, 0 }, { VK_ESCAPE, 0 }          // space, enter, escape
    };

    void Input::Clear() {
        // clean up all key states
        for (auto& keystate : keybook) {
            keystate.second = 0;
        }

        // reset cursor position to the center of window
        cursor_pos_x = Window::width * 0.5f;
        cursor_pos_y = Window::height * 0.5f;
        SetCursor(cursor_pos_x, cursor_pos_y);

        // reset cursor offsets, mouse clicks and scroll offset
        cursor_delta_x = cursor_delta_y = 0.0f;
        mouse_button_l = mouse_button_r = mouse_button_m = false;
        scroll_offset = 0.0f;
    }

    void Input::ShowCursor() {
        if constexpr (_freeglut) {
            glutSetCursor(GLUT_CURSOR_INHERIT);
        }
        else {
            glfwSetInputMode(Window::window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void Input::HideCursor() {
        if constexpr (_freeglut) {
            glutSetCursor(GLUT_CURSOR_NONE);
        }
        else {
            glfwSetInputMode(Window::window_ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    void Input::SetKeyDown(unsigned char key, bool pressed) {
        // ignore keys that are not registered in the keybook
        if (keybook.find(key) == keybook.end()) {
            return;
        }
        keybook[key] = pressed;
    }

    bool Input::GetKeyDown(unsigned char key) {
        // ignore keys that are not registered in the keybook
        if (keybook.find(key) == keybook.end()) {
            return false;
        }
        return keybook[key];
    }

    void Input::SetMouseDown(MouseButton button, bool pressed) {
        switch (button) {
            case MouseButton::Left:   mouse_button_l = pressed;  break;
            case MouseButton::Right:  mouse_button_r = pressed;  break;
            case MouseButton::Middle: mouse_button_m = pressed;  break;
            default:
                return;
        }
    }

    bool Input::GetMouseDown(MouseButton button) {
        switch (button) {
            case MouseButton::Left:   return mouse_button_l;
            case MouseButton::Right:  return mouse_button_r;
            case MouseButton::Middle: return mouse_button_m;
            default:
                return false;
        }
    }

    void Input::SetCursor(float new_x, float new_y) {
        // x and y are measured in window coordinates, with the origin at the top-left corner
        // but OpenGL uses a world coordinate system with the origin at the bottom-left corner
        cursor_delta_x = new_x - cursor_pos_x;
        cursor_delta_y = cursor_pos_y - new_y;  // invert y coordinate

        // keep cursor position fixed at the window center (required only for freeglut)
        // for GLFW, hiding the cursor will automatically lock it to the window
        if constexpr (_freeglut) {
            glutWarpPointer(cursor_pos_x, cursor_pos_y);
        }
        else {
            glfwSetCursorPos(Window::window_ptr, cursor_pos_x, cursor_pos_y);
        }
    }

    float Input::GetCursorPosition(MouseAxis axis) {
        if (axis == MouseAxis::Horizontal) {
            return cursor_pos_x;
        }
        return cursor_pos_y;
    }

    float Input::GetCursorOffset(MouseAxis axis) {
        float offset = 0.0f;

        // note that the cursor delta offsets are updated regularly by GLFW or freeglut
        // callbacks, but we have no control over exactly when and how often they are
        // called. If the scene updates much faster, we may end up calling this function
        // multiple times, before the callback has a chance to trigger again. For this
        // reason, we need to reset the cursor offset after every read in order to make
        // sure that they are never read twice.

        if (axis == MouseAxis::Horizontal) {
            offset = cursor_delta_x;
            cursor_delta_x = 0;  // reset cursor offset
        }
        else if (axis == MouseAxis::Vertical) {
            offset = cursor_delta_y;
            cursor_delta_y = 0;  // reset cursor offset
        }

        return offset;
    }

    void Input::SetScroll(float offset) {
        scroll_offset += offset;
    }

    float Input::GetScrollOffset() {
        float offset = scroll_offset;
        scroll_offset = 0.0f;  // reset scroll offset so that we never read twice
        return offset;
    }

    #pragma warning(pop)

}
