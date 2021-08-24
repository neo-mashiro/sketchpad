#include "pch.h"

#include "core/app.h"
#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "scene/renderer.h"
#include "scene/ui.h"

using namespace scene;

namespace core {

    // singleton instance accessor
    Application& Application::GetInstance() {
        // since c++11, this will be thread-safe, there's no need for manual locking
        static Application instance;
        return instance;
    }

    // check if the OpenGL context is active
    bool Application::IsContextActive() { return GetInstance().opengl_context_active; }

    #pragma warning(push)
    #pragma warning(disable : 4100)

    // OpenGL debug message callback
    static void GLAPIENTRY OnErrorMessage(GLenum source, GLenum type, GLuint id,
        GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {

        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;  // silently ignore notifications
        }

        std::string err_code;
        GLenum err;

        while ((err = glGetError()) != GL_NO_ERROR) {
            switch (err) {
                case GL_INVALID_ENUM:                  err_code = "invalid enumeration";      break;
                case GL_INVALID_VALUE:                 err_code = "invalid parameter value";  break;
                case GL_INVALID_OPERATION:             err_code = "invalid operation";        break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: err_code = "invalid framebuffer";      break;
                case GL_OUT_OF_MEMORY:                 err_code = "cannot allocate memory";   break;
                case GL_CONTEXT_LOST:                  err_code = "OpenGL context lost";      break;
                case GL_STACK_OVERFLOW:                err_code = "stack overflow";           break;
                case GL_STACK_UNDERFLOW:               err_code = "stack underflow";          break;
                default:                               err_code = "unknown error";            break;
            }

            CORE_ERROR("Internal error detected! {0:x}: {1}", err, err_code);
        }

        std::string err_source, err_type, err_level;

        switch (source) {
            case GL_DEBUG_SOURCE_API:             err_source = "OpenGL API calls";  break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   err_source = "Windows API calls"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: err_source = "shader compiler";   break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     err_source = "third party";       break;
            case GL_DEBUG_SOURCE_APPLICATION:     err_source = "main application";  break;
            case GL_DEBUG_SOURCE_OTHER:           err_source = "other";             break;
            default:                              err_source = "???";               break;
        }

        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               err_type = "error";               break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: err_type = "deprecated behavior"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  err_type = "undefined behavior";  break;
            case GL_DEBUG_TYPE_PORTABILITY:         err_type = "portability";         break;
            case GL_DEBUG_TYPE_PERFORMANCE:         err_type = "performance";         break;
            case GL_DEBUG_TYPE_OTHER:               err_type = "other";               break;
            default:                                err_type = "???";                 break;
        }

        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:         err_level = "high";         break;
            case GL_DEBUG_SEVERITY_MEDIUM:       err_level = "medium";       break;
            case GL_DEBUG_SEVERITY_LOW:          err_level = "low";          break;
            default:                             err_level = "???";          break;
        }

        CORE_ERROR("Debug message callback has been triggered!");
        CORE_ERROR("Error source:   {0}", err_source);
        CORE_ERROR("Error type:     {0}", err_type);
        CORE_ERROR("Error severity: {0}", err_level);
        CORE_TRACE("Error message:  {0}", message);

        // if the breakpoint is triggered, check the "Stack Frame" dropdown list above
        // to find out exactly which source file and line number has caused the error.
        // please also read the detailede error code and message in the console, FAQ:
        // https://www.khronos.org/opengl/wiki/OpenGL_Error

        #ifdef _DEBUG
            if (err_level == "High") {
                __debugbreak();  // this is the MSVC intrinsic
            }
        #endif
    }

    #pragma warning(pop)

    // -------------------------------------------------------------------------
    // freeglut event callbacks
    // -------------------------------------------------------------------------
    #pragma warning(push)
    #pragma warning(disable : 4100)

    void Application::Idle() {
        // this callback is ideal for continuous updates that need to be smooth.
        // this can be useful for continuous animation, physics updates and other
        // lightweight routines but the amount of computation should be minimized
        Clock::Update();
    }

    void Application::Display() {
        Renderer::DrawScene();
    }

    void Application::Entry(int state) {
        if (state == GLUT_ENTERED) {
            CORE_INFO("Cursor enters window");
        }
        else if (state == GLUT_LEFT) {
            CORE_INFO("Cursor leaves window");
        }
    }

    void Application::Keyboard(unsigned char key, int x, int y) {
        // when the windows api is on top, yield input control to the operating system
        if (Window::layer == Layer::Win32) {
            return;
        }

        Input::SetKeyState(key, true);

        // functional keys have the highest priority (application/window level)
        if (Input::IsKeyPressed(VK_ESCAPE)) {
            Window::ConfirmExit();  // if user confirmed exit, the program will terminate
            Input::SetKeyState(VK_ESCAPE, false);  // exit is cancelled, release esc key
            return;
        }

        if (Input::IsKeyPressed(VK_RETURN)) {
            Window::ToggleImGui();
            return;
        }

        // gameplay control keys have lower priority (scene/layer level)
        if (Window::layer == Layer::ImGui) {
            // current layer is ImGui, dispatch input control to ImGui
            ImGui_ImplGLUT_KeyboardFunc(key, x, y);

            // disable our own scene input control, we don't want objects to
            // move around the scene when the ImGui menu is on the top layer
            Input::SetKeyState(key, false);
        }
    }

    void Application::KeyboardUp(unsigned char key, int x, int y) {
        // when the windows api is on top, yield input control to the operating system
        if (Window::layer == Layer::Win32) {
            return;
        }

        Input::SetKeyState(key, false);

        if (Window::layer == Layer::ImGui) {
            ImGui_ImplGLUT_KeyboardUpFunc(key, x, y);
        }
    }

    void Application::Reshape(int width, int height) {
        // in this demo, we simply lock window position, size and aspect ratio
        Window::Reshape();

        // ImGui will decide how to respond to reshape by itself
        ImGui_ImplGLUT_ReshapeFunc(Window::width, Window::height);
    }

    void Application::Motion(int x, int y) {
        // this callback responds to mouse drag-and-move events, which is only used
        // when the current layer is ImGui (for moving, resizing & docking widgets)
        if (Window::layer == Layer::ImGui) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    }

    void Application::PassiveMotion(int x, int y) {
        if (Window::layer == Layer::Scene) {
            Input::SetMouseMove(x, y);
        }
        else if (Window::layer == Layer::ImGui) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    }

    void Application::Mouse(int button, int state, int x, int y) {
        if (Window::layer == Layer::ImGui) {
            ImGui_ImplGLUT_MouseFunc(button, state, x, y);
        }
        else if (Window::layer == Layer::Scene) {
            if (Input::IsScrollWheelDown(button, state)) {
                Input::SetMouseZoom(+1);
            }
            else if (Input::IsScrollWheelUp(button, state)) {
                Input::SetMouseZoom(-1);
            }
        }
    }

    void Application::Special(int key, int x, int y) {
        // this callback responds to special keys pressing events (f1, f2, numpads, direction keys)
        // note that this is only invoked every few frames, not each frame, so the update here
        // won't be smooth, you should place your continuous updates in the idle and display
        // callback to avoid jerkiness, this callback should only be reserved for setting flags.
        
        // also be aware that the keyboard callback in freeglut uses `unsigned char` for key types,
        // while the special callback uses `int` instead, so there are potential conflicts if you
        // use both callbacks for registering input key states due to unsafe type conversion.
        // for example, key "d", NumPad 4 and the left arrow key all have a value of 100 when
        // converted to `uint8_t`, if you press "d" to move right, the left arrow key will be
        // activated at the same time so the movements would cancel each other.
    }

    void Application::SpecialUp(int key, int x, int y) {
        // this callback responds to special keys releasing events (f1, f2, numpads, direction keys)
        // take notice of discrete updates and potential conflicts as in the special callback above
    }

    #pragma warning(pop)

    // -------------------------------------------------------------------------
    // core event functions
    // -------------------------------------------------------------------------
    void Application::Init() {
        gl_vendor = gl_renderer = gl_version = glsl_version = "";
        gl_texsize = gl_texsize_3d = gl_texsize_cubemap = gl_max_texture_units = 0;
        gl_n_msaa_buffers = gl_msaa_buffer_size = 0;

        opengl_context_active = false;

        std::cout << ".........\n" << std::endl;

        Log::Init();

        CORE_INFO("Initializing console logger ...");
        CORE_INFO("Initializing application window ...");

        Window::Init(Resolution::Normal);

        // create the main freeglut window
        glutInitDisplayMode(Window::display_mode);
        glutInitWindowSize(Window::width, Window::height);
        glutInitWindowPosition(Window::pos_x, Window::pos_y);

        Window::id = glutCreateWindow((Window::title).c_str());
        if (Window::id <= 0) {
            CORE_ERROR("Failed to create a window...");
            exit(EXIT_FAILURE);
        }

        // convert the window name from char* to wchar_t* in order to call the Win32 API
        // this step is necessary because we are compiling with Unicode rather than ANSI
        const char* win_char = (Window::title).c_str();
        const size_t n_char = strlen(win_char) + 1;
        std::wstring win_wchar = std::wstring(n_char, L'#');
        mbstowcs(&win_wchar[0], win_char, n_char);
        LPCWSTR win_ptr = (LPCWSTR)(wchar_t*)win_wchar.c_str();

        // set custom style for the freeglut window
        // https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
        HWND win_handle = WIN32::FindWindow(NULL, win_ptr);  // find the handle to the glut window
        LONG style = GetWindowLong(win_handle, GWL_STYLE);   // find the current window style
        style = style ^ WS_MAXIMIZEBOX ^ WS_MINIMIZEBOX;     // disable the maximize and minimize button
        SetWindowLong(win_handle, GWL_STYLE, style);         // apply the new window style

        // loading OpenGL function pointers
        if (glewInit() != GLEW_OK) {
            CORE_ERROR("Failed to initialize GLEW...");
            exit(EXIT_FAILURE);
        }

        CORE_INFO("Initializing input control system ...");
        Input::Init();
        Input::HideCursor();

        CORE_INFO("Initializing ImGui backends ...");
        ui::Init();

        CORE_INFO("Starting application debug session ...");

        // register the debug message callback
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OnErrorMessage, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

        // register freeglut event callbacks
        glutIdleFunc(Idle);
        glutDisplayFunc(Display);
        glutEntryFunc(Entry);
        glutKeyboardFunc(Keyboard);
        glutKeyboardUpFunc(KeyboardUp);
        glutMouseFunc(Mouse);
        glutReshapeFunc(Reshape);
        glutMotionFunc(Motion);
        glutPassiveMotionFunc(PassiveMotion);
        glutSpecialFunc(Special);
        glutSpecialUpFunc(SpecialUp);

        // opengl context is now active
        opengl_context_active = true;
        std::cout << std::endl;
    }

    void Application::Start() {
        Clock::Reset();
        Renderer::Init();
        Renderer::Attach("Welcome Screen");
    }

    void Application::PostEventUpdate() {
        Renderer::DrawImGui();
    }

    void Application::Clear() {
        CORE_TRACE("Shutting down application ...");

        ui::Clear();

        Renderer::Detach();
        Renderer::Free();

        Clock::Reset();
        Window::Clear();
    }
}
