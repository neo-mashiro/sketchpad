#include "canvas.h"

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

    frame_counter = { 0.0f, 0.0f, 0.0f };
    mouse = { 1280 / 2, 720 / 2, 0, 0 };
    keystate = { false, false, false, false, false, false };

    gl_vendor = gl_renderer = gl_version = glsl_version = "";
    gl_texsize = gl_texsize_3d = gl_texsize_cubemap = gl_max_texture_units = 0;
}

Canvas* Canvas::GetInstance() {
    static Canvas instance;
    return &instance;
}

void Canvas::CheckOpenGLContext(const std::string& call) {
    if (GetInstance()->opengl_context_active == false) {
        fprintf(stderr, "[FATAL] OpenGL context is not active! Method call failed: %s()\n", call.c_str());
        std::cin.get();
        exit(EXIT_FAILURE);
    }
}

void Canvas::Update() {
    auto* counter = &(GetInstance()->frame_counter);
    counter->this_frame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    counter->delta_time = counter->this_frame - counter->last_frame;
    counter->last_frame = counter->this_frame;
}

void Canvas::Idle(void) {
     GLenum err;
     while ((err = glGetError()) != GL_NO_ERROR) {
         fprintf(stderr, "OpenGL error detected: %d", err);
     }
}

void Canvas::Entry(int state) {
    if (state == GLUT_ENTERED) {
        std::cout << "\nCursor enters window" << std::endl;
    }
    else if (state == GLUT_LEFT) {
        std::cout << "\nCursor leaves window" << std::endl;
    }
}

void Canvas::Keyboard(unsigned char key, int x, int y) {
    if (key == VK_ESCAPE) {
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
                break;
        }
    }
    else if (key == VK_RETURN) {
    }
    else if (key == VK_SPACE) {

    }
    else if (key == VK_CONTROL) {

    }
    else {
        auto* kptr = &(GetInstance()->keystate);
        switch (key) {
            case 'w': kptr->u = true; break;
            case 's': kptr->d = true; break;
            case 'a': kptr->l = true; break;
            case 'd': kptr->r = true; break;
        }
    }
}

void Canvas::KeyboardUp(unsigned char key, int x, int y) {
    if (key == VK_RETURN) {

    }
    else if (key == VK_SPACE) {

    }
    else if (key == VK_CONTROL) {

    }
    else {
        auto* kptr = &(GetInstance()->keystate);
        switch (key) {
            case 'w': kptr->u = true; break;
            case 's': kptr->d = true; break;
            case 'a': kptr->l = true; break;
            case 'd': kptr->r = true; break;
        }
    }
}

void Canvas::Reshape(int width, int height) {
    auto* wptr = &(GetInstance()->window);

    // lock the window position, size and aspect ratio
    glutPositionWindow(wptr->pos_x, wptr->pos_y);
    glutReshapeWindow(wptr->width, wptr->height);

    // if you want to have different behaviors, you can change the window attributes
    // from your scene source code, by accessing Canvas::GetInstance()->window.param

    // there are also functions like glutFullScreen() but freeglut doesn't work well
    // with window management, it's also platform-dependent, so just keep it simple
}

void Canvas::PassiveMotion(int x, int y) {
    auto* mptr = &(GetInstance()->mouse);

    // x, y are measured in pixels in screen space, with the origin at the top-left corner
    // but OpenGL uses a world coordinate system with the origin at the bottom-left corner
    mptr->delta_x = x - mptr->pos_x;
    mptr->delta_y = mptr->pos_y - y;  // must invert y coordinate to flip the upside down

    // update mouse position
    mptr->pos_x = x;
    mptr->pos_y = y;
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

// special callback does respond to direction keys, but it's not invoked continuously every frame.
// when a key is held down, this is only invoked once every few frames, so if we update things like
// camera here, the updates are not smooth and would result in noticeable jerky movement.
// this callback should only be used to set global switches or flags such as key pressing states,
// other continuous updates must be done in the idle/display callback, which happens every frame.
void Canvas::Special(int key, int x, int y) {
    auto* kptr = &(GetInstance()->keystate);
    switch (key) {
        case GLUT_KEY_UP:    kptr->f = true; break;
        case GLUT_KEY_DOWN:  kptr->b = true; break;
        case GLUT_KEY_LEFT:  kptr->l = true; break;
        case GLUT_KEY_RIGHT: kptr->r = true; break;
    }
}

// this callback responds to key releasing events, where we can reset key pressing states
void Canvas::SpecialUp(int key, int x, int y) {
    auto* kptr = &(GetInstance()->keystate);
    switch (key) {
        case GLUT_KEY_UP:    kptr->f = false; break;
        case GLUT_KEY_DOWN:  kptr->b = false; break;
        case GLUT_KEY_LEFT:  kptr->l = false; break;
        case GLUT_KEY_RIGHT: kptr->r = false; break;
    }
}
