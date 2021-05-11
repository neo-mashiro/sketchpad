#include "canvas.h"

namespace Sketchpad {
    // private functions
    void OnExitConfirm(Window* wptr) {
        bool was_on_top = wptr->on_top_layer;

        if (was_on_top) {
            wptr->on_top_layer = false;
            glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor
        }

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
                if (was_on_top) {
                    wptr->on_top_layer = true;
                    glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor
                }
                break;
        }
    }

    void ToggleControlMenu(Window* wptr) {
        if (wptr->on_top_layer) {
            wptr->on_top_layer = false;
            glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor
            // TODO: open ImGUI control menu
        }
        else {
            wptr->on_top_layer = true;
            glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor
            // TODO: close ImGUI control menu
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
        window.on_top_layer = true;

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

    // check if a valid OpenGL context is present, used by other modules to validate context
    void Canvas::CheckOpenGLContext(const std::string& call) {
        if (GetInstance()->opengl_context_active == false) {
            fprintf(stderr, "[FATAL] OpenGL context is not active! Method call failed: %s()\n", call.c_str());
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

    // all the functions below are glut event callbacks

    void Canvas::Idle(void) {
         GLenum err;
         while ((err = glGetError()) != GL_NO_ERROR) {
             fprintf(stderr, "OpenGL error detected: %d", err);
         }
    }

    void Canvas::Entry(int state) {
        if (state == GLUT_ENTERED) {
            std::cout << "Cursor enters window" << std::endl;
        }
        else if (state == GLUT_LEFT) {
            std::cout << "Cursor leaves window" << std::endl;
        }
    }

    void Canvas::Keyboard(unsigned char key, int x, int y) {
        if (key == VK_ESCAPE) {
            OnExitConfirm(&(GetInstance()->window));
        }
        else if (key == VK_RETURN) {
            ToggleControlMenu(&(GetInstance()->window));
        }
        else if (key == VK_SPACE) {
            auto* kptr = &(GetInstance()->keystate);
            kptr->u = true;
        }
        else if (key == 'z') {
            auto* kptr = &(GetInstance()->keystate);
            kptr->d = true;
        }
        else {
            auto* kptr = &(GetInstance()->keystate);
            switch (key) {
                case 'w': kptr->f = true; break;
                case 's': kptr->b = true; break;
                case 'a': kptr->l = true; break;
                case 'd': kptr->r = true; break;
            }
        }
    }

    void Canvas::KeyboardUp(unsigned char key, int x, int y) {
        if (key == VK_SPACE) {
            auto* kptr = &(GetInstance()->keystate);
            kptr->u = false;
        }
        else if (key == 'z') {
            auto* kptr = &(GetInstance()->keystate);
            kptr->d = false;
        }
        else {
            auto* kptr = &(GetInstance()->keystate);
            switch (key) {
                case 'w': kptr->f = false; break;
                case 's': kptr->b = false; break;
                case 'a': kptr->l = false; break;
                case 'd': kptr->r = false; break;
            }
        }
    }

    void Canvas::Reshape(int width, int height) {
        auto* wptr = &(GetInstance()->window);

        // lock window position, size and aspect ratio
        glutPositionWindow(wptr->pos_x, wptr->pos_y);
        glutReshapeWindow(wptr->width, wptr->height);

        // if you want to have different behaviors, you can change the window attributes
        // from your scene source code, by accessing Canvas::GetInstance()->window.param

        // there are also functions like glutFullScreen() but freeglut doesn't work well
        // with window management, it's also platform-dependent, so just keep it simple
    }

    void Canvas::PassiveMotion(int x, int y) {
        auto* mptr = &(GetInstance()->mouse);
        auto* wptr = &(GetInstance()->window);

        // when scene is on top layer, detect mouse movement and keep cursor position fixed
        if (wptr->on_top_layer) {
            // x, y are measured in pixels in screen space, with the origin at the top-left corner
            // but OpenGL uses a world coordinate system with the origin at the bottom-left corner
            mptr->delta_x = x - mptr->pos_x;
            mptr->delta_y = mptr->pos_y - y;  // must invert y coordinate to flip the upside down

            glutWarpPointer(mptr->pos_x, mptr->pos_y);
        }
        // when control menu is on top, cursor is visible, update cursor position each frame
        else {
            mptr->pos_x = x;
            mptr->pos_y = y;
        }
    }

    void Canvas::Mouse(int button, int state, int x, int y) {
        auto* wptr = &(GetInstance()->window);

        // in freeglut, each scrollwheel event is reported as a button click
        if (button == 3 && state == GLUT_DOWN) {  // scroll up
            wptr->zoom = -1;
        }
        else if (button == 4 && state == GLUT_DOWN) {  // scroll down
            wptr->zoom = +1;
        }
    }

    // this callback responds to special key pressing events (f1, f2, numpads, etc.)
    // note that this is not invoked every frame, but once a few frames, whatever updates we
    // do here will not be smooth, so this should only be used to set canvas states or flags.
    // updates must be done in the idle/display callback to avoid noticeable jerky movement.
    void Canvas::Special(int key, int x, int y) {
        auto* kptr = &(GetInstance()->keystate);
        switch (key) {
            case GLUT_KEY_UP:    kptr->f = true; break;
            case GLUT_KEY_DOWN:  kptr->b = true; break;
            case GLUT_KEY_LEFT:  kptr->l = true; break;
            case GLUT_KEY_RIGHT: kptr->r = true; break;
        }
    }

    // this callback responds to special key releasing events
    void Canvas::SpecialUp(int key, int x, int y) {
        auto* kptr = &(GetInstance()->keystate);
        switch (key) {
            case GLUT_KEY_UP:    kptr->f = false; break;
            case GLUT_KEY_DOWN:  kptr->b = false; break;
            case GLUT_KEY_LEFT:  kptr->l = false; break;
            case GLUT_KEY_RIGHT: kptr->r = false; break;
        }
    }
}
