#include "pch.h"

#include <windows.h>

#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"

namespace core {

    void Window::Init(Resolution res) {
        Resize(res);
        display_mode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
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

    void Window::Resize(Resolution res) {
        resolution = res;

        if (res == Resolution::Large) {
            width = 1600;
            height = 900;
        }
        else if (res == Resolution::Normal) {
            width = 1280;
            height = 720;
        }

        pos_x = (glutGet(GLUT_SCREEN_WIDTH) - width) / 2;
        pos_y = (glutGet(GLUT_SCREEN_HEIGHT) - height) / 2;

        CORE_TRACE("Window resolution changed to {0}x{1} ...", width, height);
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

    void Window::Refresh() {
        glutSwapBuffers();
        glutPostRedisplay();
    }

    void Window::ConfirmExit() {
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
            CORE_WARN("Total application running time: {0} seconds", Clock::time);
            CORE_WARN("Shutting down the program ...");
            exit(EXIT_SUCCESS);
        }
        else if (button_id == IDCANCEL) {
            layer = cache_layer;  // recover layer
            if (cache_layer == Layer::Scene) {
                Input::HideCursor();
            }
        }
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
