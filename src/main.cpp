#include <iostream>
#include <define.h>


int window;


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(display_mode);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH) / 4, glutGet(GLUT_SCREEN_HEIGHT) / 4);

    window = glutCreateWindow(window_title);

    if (glewInit()) {
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

    return 0;
}