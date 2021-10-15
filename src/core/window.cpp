#include "pch.h"

#include <windows.h>

#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"

namespace core {

    void Window::Init() {
        Resize();
        glutSetOption(GLUT_MULTISAMPLE, 4);  // enforce 4 samples per pixel MSAA
        display_mode = GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE;
    }

    void Window::Clear() {
        if (id >= 0) {
            glutDestroyWindow(id);
            id = -1;
        }
    }

    void Window::Rename(const std::string& new_title) {
        title = new_title;
        glutSetWindowTitle(title.c_str());
    }

    void Window::Resize() {
        // currently we only support a fixed resolution of 1600 x 900 so it's not a
        // "resize" function in real sense, but we could extend this in the future
        width = 1600;
        height = 900;
        pos_x = (glutGet(GLUT_SCREEN_WIDTH) - width) / 2;
        pos_y = (glutGet(GLUT_SCREEN_HEIGHT) - height) / 2;

        CORE_TRACE("Window resolution is set to {0}x{1} ...", width, height);
    }

    void Window::Reshape() {
        // in this demo, we simply lock window position, size and aspect ratio
        glutPositionWindow(pos_x, pos_y);
        glutReshapeWindow(width, height);

        // while window position is measured in pixels in the screen space, viewport position is
        // relative to the window space, both of them use the bottom-left corner as the origin.
        // on reshape, we should place the viewport at position (0, 0) instead of (pos_x, pos_y).
        glViewport(0, 0, width, height);
    }

    bool Window::ConfirmExit() {
        // remember the current layer
        auto cache_layer = layer;

        // change layer to windows api, let the operating system take over
        layer = Layer::Win32;
        Input::ShowCursor();

        // pop up the windows exit message box
        int button_id = MessageBox(NULL,
            (LPCWSTR)L"Do you want to close the window?",
            (LPCWSTR)L"Sketchpad.exe",
            MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_SETFOREGROUND
        );

        if (button_id == IDOK) {
            glutLeaveMainLoop();
            // instead of exit directly, we should return control to the app and exit there,
            // so that all stacks will be properly unwound and all destructors are called
            return true;  // exit(EXIT_SUCCESS) will lead to memory leaks
        }
        else if (button_id == IDCANCEL) {
            layer = cache_layer;  // recover layer
            if (cache_layer == Layer::Scene) {
                Input::HideCursor();
            }
        }
        
        return false;
    }

    void Window::ToggleImGui() {
        if (layer == Layer::ImGui) {
            layer = Layer::Scene;
            Input::HideCursor();
            Input::ResetCursor();
        }
        else {
            layer = Layer::ImGui;
            Input::ShowCursor();
        }
    }
}
