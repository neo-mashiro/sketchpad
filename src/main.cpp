#include "define.h"

#pragma execution_character_set("utf-8")

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    #ifdef _DEBUG
    // if the breakpoint is triggered, check the "Stack Frame" dropdown list above
    // to find out exactly which source file and line number has caused the error,
    // also read the error message and error code in the console, then reference:
    // https://www.khronos.org/opengl/wiki/OpenGL_Error
    __debugbreak();
    #endif
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(65001);  // set the console code page to utf-8

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

    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL current version: " << glGetString(GL_VERSION) << '\n' << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);

    Init();

    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutIdleFunc(Idle);
    glutMotionFunc(Motion);
    glutPassiveMotionFunc(PassiveMotion);

    glutMainLoop();

    Cleanup();

    glutDestroyWindow(window.id);

    return 0;
}
