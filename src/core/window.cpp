#include "pch.h"

#include <windows.h>
#include <gdiplus.h>
#include <gdiplusinit.h>
#include <atlimage.h>

#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "utils/filesystem.h"

namespace core {

    static HWND hWND = nullptr;  // Win32 internal window handle

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

        // retrieve the global handle of our window, will be used by screenshots later
        hWND = ::FindWindow(NULL, (LPCWSTR)(L"sketchpad"));

        // disable max/min/close button on the title bar
        LONG style = GetWindowLong(hWND, GWL_STYLE) ^ WS_SYSMENU;
        SetWindowLong(hWND, GWL_STYLE, style);
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

    void Window::OnScreenshots() {
        /* the easiest way of taking screenshots in an OpenGL window is to read pixels from
           the framebuffer, copy them into client memory, then saves it as a PNG file using
           the `stb` library which we already have. There's no need to rely on extra tools
           such as OpenCV, with this approach, the pseudocode may look something like this:

           > #define STB_IMAGE_WRITE_IMPLEMENTATION
           > #include <stb/stb_image_write.h>
           
           > uint8_t* ptr = (uint8_t*)malloc(width * height * 4);
           > glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
           > glFinish();
           > stbi_write_png("D:\\sketchpad\\example.png", width, height, 4, ptr, 0);
           > free(ptr);

           however, reading pixels isn't very fast, and `stbi_write_png` can be especially
           slow when saving image to the disk. To make sure the pixel reads are visible, we
           might also need to call `glFenceSync()` or `glFinish()` before writing. Another
           drawback of this method is that we can only read pixels from the framebuffer, so
           we have to defer this acton to the end of frame to avoid capturing intermediate
           results, and it's also not possible to capture the entire window other than the
           content area. Due to these limitations, it would be much better to use the GDI+
           library instead which is part of the built-in Win32 API, this will work for all
           window applications, not just OpenGL.

           to take a screenshot of our window, we first capture the entire screen, and then
           extract our window's rectangle area from it. Since we know the position and size
           of our app window, and that it must be active (not necessarily on top) when this
           function gets called, this is the best and most robust option, as we don't need
           to care about the window's transparency, z-order or if it is an empty container.
           this approach is guaranteed to work, as we can always capture the desktop window.
           by not relying on the device context (DC) of our application window, we wouldn't
           fail or capture some background windows by accident.

           in order to extract the correct rectangle, we must make sure the window stays at
           its initial position before taking the screenshot. If it has been moved by the
           user, we move it back to the center of the screen. Speaking of window position,
           note that the Win32 API also takes into account the size of decorated title bars
           and border frames, not just the content area of our window. Therefore, we should
           also adjust the window size to include these decorations.
        */

        // screenshot filename format: "YYYY_MM_DD_HH24_MM_SS.PNG" (UTC time)
        static std::string last_datetime;
        std::string datetime = Clock::GetDateTimeUTC();
        std::string filename = utils::paths::screenshot + datetime + ".png";

        // allow at most one screenshot to be taken per second
        if (last_datetime == datetime) {
            return;
        }

        last_datetime = datetime;

        // convert filename to wchar_t* as we are compiling with Unicode
        const char* char_ptr = filename.c_str();
        const size_t n_char = strlen(char_ptr) + 1;
        std::wstring wchar_str = std::wstring(n_char, L'#');
        mbstowcs(&wchar_str[0], char_ptr, n_char);
        const wchar_t* wfilename = wchar_str.c_str();

        static int border = 1;     // window border thickness
        static int titlebar = 30;  // window title bar height
        static int margin = 7;     // screen left margin....

        // make sure our window is positioned at the center of screen
        UINT flags = SWP_NOZORDER | SWP_NOSIZE;
        ::SetWindowPos(hWND, 0, pos_x - border - margin, pos_y - border - titlebar, 0, 0, flags);

        // for simplicity, we use the Win32 GDI+ tool to capture the screen
        Gdiplus::GdiplusStartupInput input;
        ULONG_PTR token = 0;
        auto status = Gdiplus::GdiplusStartup(&token, &input, NULL);

        if (status != Gdiplus::Status::Ok) {
            CORE_ERROR("Failed to initialize GDI+, GdiplusStartup() returns {0}", static_cast<int>(status));
            Gdiplus::GdiplusShutdown(token);
            return;
        }

        static HDC desktopDC = GetDC(HWND_DESKTOP);  // get the desktop handle once and caches it
        HDC hDC = CreateCompatibleDC(desktopDC);
        SetStretchBltMode(hDC, COLORONCOLOR);

        static int px = pos_x - border;
        static int py = pos_y - border - titlebar;
        static int sx = 2 * border + width;
        static int sy = 2 * border + titlebar + height;

        HBITMAP bitmap = CreateCompatibleBitmap(desktopDC, sx, sy);
        SelectObject(hDC, bitmap);

        BITMAPINFOHEADER bitmap_header = { sizeof(BITMAPINFOHEADER),
            sx,      // bitmap width in pixels
            -sy,     // bitmap height in pixels (a top-down DIB uses negative height)
            1,       // must be set to 1
            32,      // number of bits per pixel (bpp)
            BI_RGB,  // uncompressed RGB
            0,       // image size in bytes (0 for uncompressed RGB bitmaps)
            0,       // horizontal resolution, in pixels per meter
            0,       // vertical resolution, in pixels per meter
            0,       // number of color indices in the color table
            0        // all color indices are considered important
        };

        static DWORD bitmap_size = sx * sy * 4;  // 32 bits is 4 bytes
        HANDLE hDIB = GlobalAlloc(GHND, bitmap_size);  // should use heap functions but who care
        LPVOID lpbitmap = nullptr;

        if (hDIB != NULL) {
            lpbitmap = GlobalLock(hDIB);
        }
        else {
            CORE_ERROR("Unable to allocate bitmap memory, Alloc() returns {0})", GetLastError());
        }

        StretchBlt(hDC, 0, 0, sx, sy, desktopDC, px, py, sx, sy, SRCCOPY);
        GetDIBits(hDC, bitmap, 0, sy, lpbitmap, (BITMAPINFO*)&bitmap_header, DIB_RGB_COLORS);

        DeleteDC(hDC);
        ReleaseDC(HWND_DESKTOP, desktopDC);

        // save the bitmap as a PNG file
        ATL::CImage image;
        image.Attach(bitmap);

        if (auto result = image.Save(wfilename); result == S_OK) {
            CORE_TRACE("Screenshot has been saved to {0}", filename);
        }
        else {
            CORE_ERROR("Failed to take a screenshot, Save() returns {0}", result);
        }

        Gdiplus::GdiplusShutdown(token);
    }

    void Window::OnOpenBrowser() {
        static const std::string repo_link = "https://github.com/neo-mashiro/sketchpad";
        std::string command = "start " + repo_link;  // for linux use "xdg-open"
        std::system(command.c_str());
    }

    bool Window::OnExitRequest() {
        // remember the current layer
        auto cache_layer = layer;

        // change layer to windows api, let the operating system take over
        layer = Layer::Win32;
        Input::ShowCursor();

        // move cursor to the position of cancel button before the message box pops up
        if constexpr (_freeglut) {
            glutWarpPointer(892, 515);
        }
        else {
            glfwSetCursorPos(Window::window_ptr, 892, 515);
        }

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

}
