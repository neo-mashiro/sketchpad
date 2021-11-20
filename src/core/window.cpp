#include "pch.h"

#include <windows.h>
#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"

namespace core {

    void Window::Init() {
        width = 1600;
        height = 900;

        if constexpr (_freeglut) {
            pos_x = (glutGet(GLUT_SCREEN_WIDTH) - width) / 2;
            pos_y = (glutGet(GLUT_SCREEN_HEIGHT) - height) / 2;

            if constexpr (debug_mode) {
                glutInitContextFlags(GLUT_DEBUG);  // hint the debug context
            }

            glutSetOption(GLUT_MULTISAMPLE, 4);  // enforce 4 samples per pixel MSAA
            glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
            glutInitWindowSize(width, height);
            glutInitWindowPosition(pos_x, pos_y);

            window_id = glutCreateWindow(title.c_str());
            CORE_ASERT(window_id > 0, "Failed to create the main window ...");
            CORE_INFO("Window resolution is set to {0}x{1} ...", width, height);
        }

        else {
            const GLFWvidmode* vmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            pos_x = (vmode->width - width) / 2;
            pos_y = (vmode->height - height) / 2;

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_SAMPLES, 4);  // enforce 4 samples per pixel MSAA

            if constexpr (debug_mode) {
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);  // hint the debug context
            }

            window_ptr = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
            CORE_ASERT(window_ptr != nullptr, "Failed to create the main window...");
            CORE_INFO("Window resolution is set to {0}x{1} ...", width, height);

            glfwSetWindowPos(window_ptr, pos_x, pos_y);
            glfwSetWindowAspectRatio(window_ptr, 16, 9);
            glfwSetWindowAttrib(window_ptr, GLFW_RESIZABLE, GLFW_FALSE);

            glfwMakeContextCurrent(window_ptr);
            glfwSwapInterval(0);  // disable vsync because we want to test performance
        }

        // disable max/min/close button on the title bar
        HWND win_handle = WIN32::FindWindow(NULL, (LPCWSTR)(L"sketchpad"));
        LONG style = GetWindowLong(win_handle, GWL_STYLE) ^ WS_SYSMENU;
        SetWindowLong(win_handle, GWL_STYLE, style);
    }

    void Window::Clear() {
        if constexpr (_freeglut) {
            if (window_id > 0) {
                glutDestroyWindow(window_id);
                window_id = 0;
            }
        }
        else {
            if (window_ptr) {
                glfwDestroyWindow(window_ptr);
                glfwTerminate();
                window_ptr = nullptr;
            }
        }
    }

    void Window::Rename(const std::string& new_title) {
        title = new_title;
        if constexpr (_freeglut) {
            glutSetWindowTitle(title.c_str());
        }
        else {
            glfwSetWindowTitle(window_ptr, title.c_str());
        }
    }

    void Window::Resize() {
        // in this demo, we simply lock window position, size and aspect ratio
        if constexpr (_freeglut) {
            glutPositionWindow(pos_x, pos_y);
            glutReshapeWindow(width, height);
        }
        else {
            glfwSetWindowPos(window_ptr, pos_x, pos_y);
            glfwSetWindowSize(window_ptr, width, height);
            glfwSetWindowAspectRatio(window_ptr, 16, 9);
        }

        // viewport position is in pixels, relative to the the bottom-left corner of the window
        glViewport(0, 0, width, height);
    }

    bool Window::OnExitRequest() {
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
            // return control to the app and exit there, so that we have a chance to
            // clean things up, if we exit here directly, there will be memory leaks
            return true;
        }
        else if (button_id == IDCANCEL) {
            layer = cache_layer;  // recover layer
            if (cache_layer == Layer::Scene) {
                Input::HideCursor();
            }
        }
        
        return false;
    }

    void Window::OnLayerSwitch() {
        layer = (layer == Layer::ImGui) ? Layer::Scene : Layer::ImGui;
        if (layer == Layer::ImGui) {
            Input::ShowCursor();
        }
        else {
            Input::HideCursor();
            Input::Clear();
        }
    }
}
