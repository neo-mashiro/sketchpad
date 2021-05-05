#include "canvas.h"

Canvas::Canvas(const std::string& title, unsigned int width, unsigned int height) {
    window = { -1, title, width, height, 1.0f, 0, 0, 0, 0 };
    window.display_mode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
    window.aspect_ratio = (float)width / height;
    window.pos_x = (glutGet(GLUT_SCREEN_WIDTH) - width) / 2;
    window.pos_y = (glutGet(GLUT_SCREEN_HEIGHT) - height) / 2;

    frame_counter = { 0.0f, 0.0f, 0.0f };
    mouse = { window.width / 2, window.height / 2, 0, 0 };
    keystate = { false, false, false, false };
}

Canvas::~Canvas() {
    glutDestroyWindow(window.id);
    window.id = -1;
}

void Canvas::Update() {
    frame_counter.this_frame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    frame_counter.delta_time = frame_counter.this_frame - frame_counter.last_frame;
    frame_counter.last_frame = frame_counter.this_frame;
}

void Canvas::Idle(void) {}

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
}

void Canvas::Reshape(int width, int height) {
    // update window size (but keep the original aspect ratio)
    window.width = width;
    window.height = height;

    // viewport has the same aspect ratio as the window
    GLsizei viewport_w = width <= height ? width : height;
    GLsizei viewport_h = viewport_w / window.aspect_ratio;

    // center viewport relative to the window
    GLint vpos_x = (width - viewport_w) / 2;
    GLint vpos_y = (height - viewport_h) / 2;

    glViewport(vpos_x, vpos_y, viewport_w, viewport_h);
}

void Canvas::PassiveMotion(int x, int y) {
    // x, y are measured in pixels in screen space, with the origin at the top-left corner
    // but OpenGL uses a world coordinate system with the origin at the bottom-left corner
    mouse.delta_x = x - mouse.pos_x;
    mouse.delta_y = mouse.pos_y - y;  // must invert y coordinate to flip the upside down

    // update mouse position
    mouse.pos_x = x;
    mouse.pos_y = y;
}

void Canvas::Mouse(int button, int state, int x, int y) {
    // in freeglut, each scrollwheel event is reported as a button click
    if (button == 3 && state == GLUT_DOWN) {  // scroll up
        window.zoom = -1;
    }
    else if (button == 4 && state == GLUT_DOWN) {  // scroll down
        window.zoom = +1;
    }
}

// special callback does respond to direction keys, but it's not invoked continuously every frame.
// when a key is held down, this is only invoked once every few frames, so if we update things like
// camera here, the updates are not smooth and would result in noticeable jerky movement.
// this callback should only be used to set global switches or flags such as key pressing states,
// other continuous updates must be done in the idle/display callback, which happens every frame.
void Canvas::Special(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:    keystate.u = true; break;
        case GLUT_KEY_DOWN:  keystate.d = true; break;
        case GLUT_KEY_LEFT:  keystate.l = true; break;
        case GLUT_KEY_RIGHT: keystate.r = true; break;
    }
}

// this callback responds to key releasing events, where we can reset key pressing states
void Canvas::SpecialUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:    keystate.u = false; break;
        case GLUT_KEY_DOWN:  keystate.d = false; break;
        case GLUT_KEY_LEFT:  keystate.l = false; break;
        case GLUT_KEY_RIGHT: keystate.r = false; break;
    }
}
