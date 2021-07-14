#include "pch.h"

#include <windows.h>

#include "core/app.h"
#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"

#include "scene_01/scene_01.h"
#include "scene_02/scene_02.h"

namespace core {

    // singleton instance accessor
    Application& Application::GetInstance() {
        static Application instance;
        return instance;
    }

    // check if a valid OpenGL context is present, used by other modules to validate context
    void Application::CheckOpenGLContext(const std::string& call) {
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
            // pop up the exit confirmation message box
            Layer layer = Window::layer;
            Window::layer = Layer::Win32;

            glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor

            int button_id = MessageBox(NULL,
                (LPCWSTR)L"Do you want to close the window?",
                (LPCWSTR)L"Sketchpad.exe",
                MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_SETFOREGROUND
            );

            if (button_id == IDOK) {
                glutLeaveMainLoop();
                CORE_INFO("Shutting down application ...");
                exit(EXIT_SUCCESS);
            }
            else if (button_id == IDCANCEL) {
                Input::SetKeyState(VK_ESCAPE, false);  // release Enter key
                Window::layer = layer;  // recover layer

                if (layer == Layer::Scene) {
                    glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor
                }
            }

            return;
        }

        if (Input::IsKeyPressed(VK_RETURN)) {
            // toggle the ImGui menu layer
            if (Window::layer == Layer::ImGui) {
                Window::layer = Layer::Scene;
                glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor
            }
            else {
                Window::layer = Layer::ImGui;
                glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor
            }

            return;
        }

        // gameplay control keys have lower priority (scene/layer level)
        if (Window::layer == Layer::ImGui) {
            // current layer is ImGui, transfer input control to ImGui
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
        // this callback responds to special keys pressing events (f1, f2, numpads)
        // it's only invoked every few frames, not each frame, so the update here
        // won't be smooth, this callback should only be used to set input states.
        // in contrast, the idle and display callback is guaranteed to be called
        // every frame, place your continuous updates there to avoid jerkiness.
        if (Window::layer == Layer::Win32) {
            return;
        }

        if (Window::layer == Layer::ImGui) {
            ImGui_ImplGLUT_SpecialFunc(key, x, y);
        }
        else if (Window::layer == Layer::Scene) {
            Input::SetKeyState(key, true);
        }
    }

    void Application::SpecialUp(int key, int x, int y) {
        // this callback responds to special keys releasing events (f1, f2, numpads)
        if (Window::layer == Layer::Win32) {
            return;
        }

        Input::SetKeyState(key, false);

        if (Window::layer == Layer::ImGui) {
            ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
        }
    }

    #pragma warning(pop)

    // -------------------------------------------------------------------------
    // core event functions
    // -------------------------------------------------------------------------
    // application initializer
    void Application::Init(Scene* scene) {
        gl_vendor = gl_renderer = gl_version = glsl_version = "";
        gl_texsize = gl_texsize_3d = gl_texsize_cubemap = gl_max_texture_units = 0;

        opengl_context_active = false;
        active_scene = nullptr;

        // initialize spdlog logger
        Log::Init();

        // create the main window
        Window::Init();
        glutInitDisplayMode(Window::display_mode);
        glutInitWindowSize(Window::width, Window::height);
        glutInitWindowPosition(Window::pos_x, Window::pos_y);

        Window::id = glutCreateWindow(Window::title);

        if (Window::id <= 0) {
            CORE_ERROR("Failed to create a window...");
            exit(EXIT_FAILURE);
        }

        // hide cursor inside the window
        glutSetCursor(GLUT_CURSOR_NONE);

        // load opengl core library from hardware
        if (glewInit() != GLEW_OK) {
            CORE_ERROR("Failed to initialize GLEW...");
            exit(EXIT_FAILURE);
        }

        // initialize ImGui backends, create ImGui context
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::StyleColorsDark();

        ImGui_ImplGLUT_Init();
        ImGui_ImplOpenGL3_Init();

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

        // initialize the active scene
        active_scene = scene;
        active_scene->Init();
        Clock::Reset();
    }

    // post-update after each iteration of the freeglut event loop
    void Application::PostEventUpdate() {
        Clock::Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGLUT_NewFrame();

        // the upper left menu bar is always rendered on top of all scenes and layers,
        // which allows us to have application level control over the scenes (switch)
        ImGui::Begin("Test Switch Scene", 0, ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Open Scene")) {
                if (ImGui::MenuItem("Colorful Cubes", "Ctrl+O")) {
                    Switch(new Scene01());
                }

                if (ImGui::MenuItem("Skybox Reflection", "Ctrl+S")) {
                    Switch(new Scene02());
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();

        if (Window::layer == Layer::ImGui) {
            active_scene->OnImGuiRender();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glutSwapBuffers();
        glutPostRedisplay();
    }

    // switch to a new scene
    void Application::Switch(Scene* scene) {
        // make sure to safely clear the OpenGL states held by previous scene objects
        // the Scene object should be created using the new operator and then delete
        // explicitly so that its destructor will call all other class destructors which
        // will safely clean up the internal OpenGL states

        // TODO:
        // smoothly switch scenes by using a loading screen

        delete active_scene;
        active_scene = scene;
        active_scene->Init();
    }

    // clean up after the last frame
    void Application::Clear() {
        active_scene = nullptr;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
    }
}
