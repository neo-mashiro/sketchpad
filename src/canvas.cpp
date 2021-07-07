#include "canvas.h"
#include "log.h"

namespace Sketchpad {
    // private functions
    static void OnExitConfirm(Window* wptr) {
        WindowLayer layer = wptr->current_layer;
        wptr->current_layer = WindowLayer::win32;

        glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor

        int button_id = MessageBox(NULL,
            (LPCWSTR)L"Do you want to close the window?",
            (LPCWSTR)L"Sketchpad Canvas",
            MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_SETFOREGROUND
        );

        switch (button_id) {
            case IDOK:
                glutLeaveMainLoop();
                break;
            case IDCANCEL:
                wptr->current_layer = layer;  // recover layer
                if (layer == WindowLayer::scene) {
                    glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor
                }
                break;
        }
    }

    static void ToggleControlMenu(Window* wptr) {
        if (wptr->current_layer == WindowLayer::imgui) {
            wptr->current_layer = WindowLayer::scene;
            glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor
        }
        else {
            wptr->current_layer = WindowLayer::imgui;
            glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor
        }
    }

    // private constructor
    Canvas::Canvas() {
        opengl_context_active = false;

        window = { -1, "sketchpad", 1280, 720, 1280, 720, 1.0f, 0, 0, 0, 0 };
        window.aspect_ratio = 1280.0f / 720.0f;
        window.full_width = glutGet(GLUT_SCREEN_WIDTH);
        window.full_height = glutGet(GLUT_SCREEN_HEIGHT);
        window.pos_x = (window.full_width - 1280) / 2;
        window.pos_y = (window.full_height - 720) / 2;
        window.display_mode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
        window.current_layer = WindowLayer::scene;

        frame_counter = { 0.0f, 0.0f, 0.0f, 0.0f };
        mouse = { 1280 / 2, 720 / 2, 0, 0 };
        keystate = { false, false, false, false, false, false };

        gl_vendor = gl_renderer = gl_version = glsl_version = "";
        gl_texsize = gl_texsize_3d = gl_texsize_cubemap = gl_max_texture_units = 0;
    }

    // retrieve the global singleton canvas
    Canvas* Canvas::GetInstance() {
        static Canvas instance;
        return &instance;
    }

    // initialize imgui backends and setup options
    void Canvas::CreateImGuiContext() {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        ImGui::StyleColorsDark();

        ImGui_ImplGLUT_Init();
        ImGui_ImplOpenGL3_Init();
    }

    // check if a valid OpenGL context is present, used by other modules to validate context
    void Canvas::CheckOpenGLContext(const std::string& call) {
        if (GetInstance()->opengl_context_active == false) {
            CORE_ERROR("OpenGL context is not active! Method call failed: {0}()", call.c_str());
            std::cin.get();
            exit(EXIT_FAILURE);
        }
    }

    // keep track of the frame statistics, all scene updates depend on this
    void Canvas::Update() {
        auto* counter = &(GetInstance()->frame_counter);
        counter->this_frame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        counter->delta_time = counter->this_frame - counter->last_frame;
        counter->last_frame = counter->this_frame;
        counter->time += counter->delta_time;
    }

    // clean up the canvas
    void Canvas::Clear() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
    }

    // all the functions below are glut event callbacks

    #pragma warning(push)
    #pragma warning(disable : 4100)

    void Canvas::Idle(void) {
         GLenum err;
         while ((err = glGetError()) != GL_NO_ERROR) {
             CORE_ERROR("OpenGL internal error detected: {0}", err);
         }
    }

    void Canvas::Entry(int state) {
        if (state == GLUT_ENTERED) {
            CORE_INFO("Cursor enters window");
        }
        else if (state == GLUT_LEFT) {
            CORE_INFO("Cursor leaves window");
        }
    }

    void Canvas::Keyboard(unsigned char key, int x, int y) {
        auto* wptr = &(GetInstance()->window);
        auto* kptr = &(GetInstance()->keystate);

        // imgui menu is the current layer, enable imgui input control
        if (wptr->current_layer == WindowLayer::imgui) {
            switch (key) {
                case VK_ESCAPE: OnExitConfirm(wptr); break;
                case VK_RETURN: ToggleControlMenu(wptr); break;
                default: ImGui_ImplGLUT_KeyboardFunc(key, x, y); break;
            }
        }
        // for the scene view, use our own input control
        else if (wptr->current_layer == WindowLayer::scene) {
            switch (key) {
                case VK_ESCAPE: OnExitConfirm(wptr); break;
                case VK_RETURN: ToggleControlMenu(wptr); break;
                case VK_SPACE: kptr->u = true; break;
                case 'z': kptr->d = true; break;
                case 'w': kptr->f = true; break;
                case 's': kptr->b = true; break;
                case 'a': kptr->l = true; break;
                case 'd': kptr->r = true; break;
            }
        }
        // when the windows api is on top, yield input control to the operating system
        else if (wptr->current_layer == WindowLayer::win32) {
            (void)x; (void)y;  // do nothing
        }
    }

    void Canvas::KeyboardUp(unsigned char key, int x, int y) {
        auto* wptr = &(GetInstance()->window);
        auto* kptr = &(GetInstance()->keystate);

        if (wptr->current_layer == WindowLayer::imgui) {
            ImGui_ImplGLUT_KeyboardUpFunc(key, x, y);
        }
        else if (wptr->current_layer == WindowLayer::scene) {
            switch (key) {
                case VK_SPACE: kptr->u = false; break;
                case 'z': kptr->d = false; break;
                case 'w': kptr->f = false; break;
                case 's': kptr->b = false; break;
                case 'a': kptr->l = false; break;
                case 'd': kptr->r = false; break;
            }
        }
        else if (wptr->current_layer == WindowLayer::win32) {
            (void)x; (void)y;  // do nothing
        }
    }

    void Canvas::Reshape(int width, int height) {
        auto* wptr = &(GetInstance()->window);

        // lock window position, size and aspect ratio
        glutPositionWindow(wptr->pos_x, wptr->pos_y);
        glutReshapeWindow(wptr->width, wptr->height);

        ImGui_ImplGLUT_ReshapeFunc(wptr->width, wptr->height);

        // if you want to have different behaviors, you can change the window attributes
        // from your scene source code, by accessing Canvas::GetInstance()->window.param

        // there are also functions like glutFullScreen() but freeglut doesn't work well
        // with window management, it's also platform-dependent, so just keep it simple
    }

    void Canvas::Motion(int x, int y) {
        // this callback responds to mouse drag-and-move events, which is only used
        // when the current layer is ImGui (for moving, resizing & docking widgets)
        auto* wptr = &(GetInstance()->window);
        if (wptr->current_layer == WindowLayer::imgui) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    }

    void Canvas::PassiveMotion(int x, int y) {
        auto* mptr = &(GetInstance()->mouse);
        auto* wptr = &(GetInstance()->window);

        // scene is the current layer, detect mouse movement and keep cursor position fixed
        if (wptr->current_layer == WindowLayer::scene) {
            // x, y are measured in pixels in screen space, with the origin at the top-left corner
            // but OpenGL uses a world coordinate system with the origin at the bottom-left corner
            mptr->delta_x = x - mptr->pos_x;
            mptr->delta_y = mptr->pos_y - y;  // must invert y coordinate to flip the upside down

            glutWarpPointer(mptr->pos_x, mptr->pos_y);
        }
        // imgui layer is on top, cursor is visible, update cursor position each frame
        else if (wptr->current_layer == WindowLayer::imgui) {
            mptr->pos_x = x;
            mptr->pos_y = y;

            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    }

    void Canvas::Mouse(int button, int state, int x, int y) {
        auto* wptr = &(GetInstance()->window);

        if (wptr->current_layer == WindowLayer::imgui) {
            ImGui_ImplGLUT_MouseFunc(button, state, x, y);
        }
        else if (wptr->current_layer == WindowLayer::scene) {
            // in freeglut, each scrollwheel event is reported as a button click
            if (button == 3 && state == GLUT_DOWN) {  // scroll up
                wptr->zoom = -1;
            }
            else if (button == 4 && state == GLUT_DOWN) {  // scroll down
                wptr->zoom = +1;
            }
        }
    }

    // this callback responds to special key pressing events (f1, f2, numpads, etc.)
    // note that this is not invoked every frame, but once a few frames, whatever updates we
    // do here will not be smooth, so this should only be used to set canvas states or flags.
    // updates must be done in the idle/display callback to avoid noticeable jerky movement.
    void Canvas::Special(int key, int x, int y) {
        auto* wptr = &(GetInstance()->window);
        auto* kptr = &(GetInstance()->keystate);

        if (wptr->current_layer == WindowLayer::imgui) {
            ImGui_ImplGLUT_SpecialFunc(key, x, y);
        }
        else if (wptr->current_layer == WindowLayer::scene) {
            switch (key) {
                case GLUT_KEY_UP:    kptr->f = true; break;
                case GLUT_KEY_DOWN:  kptr->b = true; break;
                case GLUT_KEY_LEFT:  kptr->l = true; break;
                case GLUT_KEY_RIGHT: kptr->r = true; break;
            }
        }
    }

    // this callback responds to special key releasing events
    void Canvas::SpecialUp(int key, int x, int y) {
        auto* wptr = &(GetInstance()->window);
        auto* kptr = &(GetInstance()->keystate);

        if (wptr->current_layer == WindowLayer::imgui) {
            ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
        }
        else if (wptr->current_layer == WindowLayer::scene) {
            switch (key) {
                case GLUT_KEY_UP:    kptr->f = false; break;
                case GLUT_KEY_DOWN:  kptr->b = false; break;
                case GLUT_KEY_LEFT:  kptr->l = false; break;
                case GLUT_KEY_RIGHT: kptr->r = false; break;
            }
        }
    }

    #pragma warning(pop)
}
