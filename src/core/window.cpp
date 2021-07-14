#include "pch.h"
#include "window.h"

namespace core {

    void Window::Init() {
        pos_x = (glutGet(GLUT_SCREEN_WIDTH) - width) / 2;
        pos_y = (glutGet(GLUT_SCREEN_HEIGHT) - height) / 2;
        display_mode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
    }

    void Window::Reshape() {
        // in this demo, we simply lock window position, size and aspect ratio
        // if you want different behaviors, change the window attributes in your own scene code
        glutPositionWindow(pos_x, pos_y);
        glutReshapeWindow(width, height);
    }
}
