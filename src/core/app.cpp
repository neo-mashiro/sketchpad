#include "pch.h"

#include <future>

#include "core/app.h"
#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "utils/ui.h"
#include "utils/factory.h"

namespace core {

    #ifdef MULTI_THREAD
    static std::future<void> _load_task;
    #endif

    // singleton instance accessor
    Application& Application::GetInstance() {
        // since c++11, this will be thread-safe, there's no need for manual locking
        static Application instance;
        return instance;
    }

    // check if a valid OpenGL context is present, used by other modules to validate context
    void Application::CheckOpenGLContext(const std::string& call) const {
        if (opengl_context_active == false) {
            CORE_ERROR("OpenGL context is not active! Method call failed: {0}()", call.c_str());
            std::cin.get();
            exit(EXIT_FAILURE);
        }
    }

    #pragma warning(push)
    #pragma warning(disable : 4100)

    // OpenGL debug message callback
    static void GLAPIENTRY OnErrorMessage(GLenum source, GLenum type,
        GLuint id, GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam) {

        fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);

        // if the breakpoint is triggered, check the "Stack Frame" dropdown list above
        // to find out exactly which source file and line number has caused the error,
        // also read the error message and error code in the console, then reference:
        // https://www.khronos.org/opengl/wiki/OpenGL_Error
        #ifdef _DEBUG
            __debugbreak();  // this is the MSVC intrinsic
        #endif
    }

    #pragma warning(pop)

    // -------------------------------------------------------------------------
    // freeglut event callbacks
    // -------------------------------------------------------------------------
    #pragma warning(push)
    #pragma warning(disable : 4100)

    void Application::Idle() {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            CORE_ERROR("OpenGL internal error detected: {0}", err);
        }
    }

    void Application::Display() {
        GetInstance().active_scene->OnSceneRender();
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
    // application initializer
    void Application::Init() {
        gl_vendor = gl_renderer = gl_version = glsl_version = "";
        gl_texsize = gl_texsize_3d = gl_texsize_cubemap = gl_max_texture_units = 0;

        opengl_context_active = false;
        last_scene = nullptr;
        active_scene = nullptr;

        // initialize spdlog logger, window attributes
        Log::Init();
        Window::Init();

        // create the main freeglut window
        glutInitDisplayMode(Window::display_mode);
        glutInitWindowSize(Window::width, Window::height);
        glutInitWindowPosition(Window::pos_x, Window::pos_y);

        Window::id = glutCreateWindow((Window::title).c_str());
        Input::HideCursor();

        if (Window::id <= 0) {
            CORE_ERROR("Failed to create a window...");
            exit(EXIT_FAILURE);
        }

        // load opengl core library from hardware
        if (glewInit() != GLEW_OK) {
            CORE_ERROR("Failed to initialize GLEW...");
            exit(EXIT_FAILURE);
        }

        // initialize ImGui backends, create ImGui context
        ui::Init();

        // register the debug message callback
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OnErrorMessage, 0);

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
    }

    // kick start the default scene
    void Application::Start() {
        CORE_INFO("Initializing welcome screen ......");
        active_scene = factory::LoadScene("Welcome Screen");
        active_scene->Init();
        Clock::Reset();
    }

    // post-update after each iteration of the freeglut event loop
    void Application::PostEventUpdate() {
        Clock::Update();
        ui::NewFrame();

        // draw application-level widget: the top menu bar
        std::string new_title = "";
        ui::DrawMenuBar(active_scene->title, new_title);

        // draw application-level widget: the bottom status bar
        ui::DrawStatusBar();

        // draw scene-level widgets if the current layer is ImGui
        if (Window::layer == Layer::ImGui && new_title.empty()) {
            active_scene->OnImGuiRender();
        }

        // switching scenes will block the main thread, but here we have a chance
        // to draw the loading screen before refreshing the window display buffer.
        if (!new_title.empty()) {
            ui::DrawLoadingScreen();
        }

        ui::EndFrame();
        Window::Refresh();

        if (!new_title.empty()) {
            Switch(new_title);  // blocking call
        }
    }

    // switch to a new scene, return only after the new scene is loaded
    void Application::Switch(const std::string& title) {
        last_scene = active_scene;
        active_scene = nullptr;

        // at any given time, there should be only one active scene, no two scenes
        // can be alive at the same time in the opengl context, so, each time we
        // switch scenes, we must delete the previous one first to safely clean up
        // global opengl states, before creating the new one from factory.

        // the new scene must be fully loaded and initialized before being assigned
        // to active_scene, otherwise, active_scene could be pointing to a scene
        // that has dirty states, so a subsequent draw call could possibly throw an
        // access violation exception that crashes the program.

        // cleaning and loading scenes can take quite a while, especially when there
        // are complicated meshes. Ideally, this function should be scheduled as an
        // asynchronous background task that runs in another thread, so as not to
        // block and freeze the window. To do so, we can wrap the task in std::async,
        // and then query the result from the std::future object, c++ will handle
        // concurrency for us, just make sure that the lifespan of the future extend
        // beyond the calling function. Sadly though, multi-threading in OpenGL is a
        // pain, you can't multithread OpenGL calls easily without using complex
        // context switching, because lots of buffer data cannot be shared between
        // threads, and freeglut or your graphics card driver may not be supporting
        // it. Sharing context and resources between threads is not worth the effort,
        // if at all possible, but you sure can load images and compute textures
        // concurrently. If you must use multiple threads, consider using DirectX.

        #ifdef MULTI_THREAD

        auto safe_load = [this](std::string title) {
            delete last_scene;
            last_scene = nullptr;
            Scene* new_scene = factory::LoadScene(title);
            new_scene->Init();
            active_scene = new_scene;
        };

        _load_task = std::async(std::launch::async, safe_load, title);

        #else

        delete last_scene;  // each object in the scene will be destructed
        last_scene = nullptr;
        Scene* new_scene = factory::LoadScene(title);
        new_scene->Init();
        active_scene = new_scene;

        #endif
    }

    // clean up after the last frame
    void Application::Clear() {
        delete active_scene;
        last_scene = nullptr;
        active_scene = nullptr;

        ui::Clear();

        Clock::Reset();
        Window::Clear();
    }
}
