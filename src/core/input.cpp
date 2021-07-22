#include "pch.h"
#include "core/input.h"

namespace core {

    #pragma warning(push)
    #pragma warning(disable : 4244)

    // keybook is a global variable (internal linkage), only visible in this translation unit.
    // the reason we define it here in the cpp file, rather than the header, is to prevent
    // other translation units from having their own separate copies of this variable.

    // keybook, being an application-level state table, is essentially independent of our
    // input class, so it should not be declared as a member variable of the class.
    static std::unordered_map<uint8_t, bool> keybook {
        { 'w', 0 }, { 's', 0 }, { 'a', 0 }, { 'd', 0 }, { 'z', 0 },  // WASD + Z
        { GLUT_KEY_UP, 0 }, { GLUT_KEY_DOWN, 0 }, { GLUT_KEY_LEFT, 0 }, { GLUT_KEY_RIGHT, 0 },  // arrow keys
        { VK_SPACE, 0 }, { VK_RETURN, 0 }, { VK_ESCAPE, 0 }  // space, enter, escape
    };

    // if we need to access keybook in other translation units, we can define it with
    // the `extern` keyword so that it has external linkage.
    extern std::unordered_map<uint8_t, bool> another_keybook {};

    bool Input::IsKeyPressed(unsigned char key) {
        return keybook[key];
    }

    // in freeglut, each scrollwheel event is reported as a button click
    bool Input::IsScrollWheelDown(int button, int state) {
        return button == 4 && state == GLUT_DOWN;
    }

    bool Input::IsScrollWheelUp(int button, int state) {
        return button == 3 && state == GLUT_DOWN;
    }

    // read mouse movement in the last frame (on axis x and y), reset after read
    GLuint Input::ReadMouseAxis(Axis axis) {
        int offset = 0;

        if (axis == Axis::X) {
            offset = mouse_delta_x;
            mouse_delta_x = 0;  // recover mouse offset
        }
        else if (axis == Axis::Y) {
            offset = mouse_delta_y;
            mouse_delta_y = 0;  // recover mouse offset
        }

        return offset;
    }

    // read mouse zoom in the last frame, reset after read
    float Input::ReadMouseZoom() {
        float zoom = mouse_zoom;
        mouse_zoom = 0.0f;  // recover zoom to 0
        return zoom;
    }

    // detect keypress events (up or down) and set key state
    void Input::SetKeyState(unsigned char key, bool pressed) {
        // ignore keys that are not registered in the keybook
        if (keybook.find(key) == keybook.end()) {
            return;
        }

        // set pressed state if and only if key is registered
        keybook[key] = pressed;
    }

    // detect mouse movement on the x and y axis and cache it
    void Input::SetMouseMove(int new_x, int new_y) {
        // x, y are measured in pixels in screen space, with the origin at the top-left corner
        // but OpenGL uses a world coordinate system with the origin at the bottom-left corner
        mouse_delta_x = new_x - mouse_pos_x;
        mouse_delta_y = mouse_pos_y - new_y;  // must invert y coordinate to flip the upside down

        // keep cursor position fixed at the center of window
        glutWarpPointer(mouse_pos_x, mouse_pos_y);
    }

    // detect mouse zoom (scrollwheel up or down) and cache it
    void Input::SetMouseZoom(float delta_zoom) {
        mouse_zoom += delta_zoom;
    }

    #pragma warning(pop)

    void Input::ShowCursor() {
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }

    void Input::HideCursor() {
        glutSetCursor(GLUT_CURSOR_NONE);
    }

    // reset cursor position to the center of window
    void Input::ResetCursor() {
        glutWarpPointer(mouse_pos_x, mouse_pos_y);
    }
}
