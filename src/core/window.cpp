#include "pch.h"

#include <windows.h>  // Windows API

#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"

namespace core {

    void Window::Init() {
        pos_x = (glutGet(GLUT_SCREEN_WIDTH) - width) / 2;
        pos_y = (glutGet(GLUT_SCREEN_HEIGHT) - height) / 2;
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

    void Window::Reshape() {
        // in this demo, we simply lock window position, size and aspect ratio
        // if you want different behaviors, change the window attributes in your own scene code
        glutPositionWindow(pos_x, pos_y);
        glutReshapeWindow(width, height);
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
