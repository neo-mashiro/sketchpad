#include "define.h"

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    SetupWindow();
    glutInitDisplayMode(window.display_mode);
    glutInitWindowSize(window.width, window.height);
    glutInitWindowPosition(window.pos_x, window.pos_y);

    window.id = glutCreateWindow(window.title);

    if (window.id <= 0) {
        std::cerr << "Unable to create a window..." << std::endl;
        exit(EXIT_FAILURE);
    }

    if (glewInit() != GLEW_OK) {
        std::cerr << "Unable to initialize GLEW..." << std::endl;
        exit(EXIT_FAILURE);
    }

    Init();

    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutIdleFunc(Idle);
    glutMotionFunc(Motion);
    glutPassiveMotionFunc(PassiveMotion);

    glutMainLoop();

    glDeleteProgram(PO);
    glutDestroyWindow(window.id);

    return 0;
}
