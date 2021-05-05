#include "canvas.h"


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
    __debugbreak();  // this is the MSVC intrinsic
    #endif
}

std::string ParseDir(const std::string& file_path) {
    return file_path.substr(0, file_path.rfind("\\")) + "\\";
}

extern Canvas canvas;

int main(int argc, char** argv) {
    SetConsoleOutputCP(65001);  // set the console code page to utf-8

    glutInit(&argc, argv);
    glutInitDisplayMode(canvas.window.display_mode);
    glutInitWindowSize(canvas.window.width, canvas.window.height);
    glutInitWindowPosition(canvas.window.pos_x, canvas.window.pos_y);

    canvas.window.id = glutCreateWindow(canvas.window.title.c_str());

    if (canvas.window.id <= 0) {
        std::cerr << "Unable to create a window..." << std::endl;
        exit(EXIT_FAILURE);
    }

    if (glewInit() != GLEW_OK) {
        std::cerr << "Unable to initialize GLEW..." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "GPU vendor name: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL current version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL primary version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n' << std::endl;

    int texture_size[3];
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_size[0]);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &texture_size[1]);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &texture_size[2]);

    std::cout << "Maximum texture size supported by this GPU: " << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << "1D/2D texture (width and height): " << texture_size[0] << std::endl;
    std::cout << "3D texture (width, height & depth): " << texture_size[1] << std::endl;
    std::cout << "Cubemap texture (width and height): " << texture_size[2] << '\n' << std::endl;

    int texture_units;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_units);
    std::cout << "Maximum number of samplers supported in the fragment shader: " << texture_units << '\n' << std::endl;

    // register debug callback
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);

    Init();

    virtual void Reshape(int width, int height);
    virtual void PassiveMotion(int x, int y);
    virtual void Mouse(int button, int state, int x, int y);

    glutDisplayFunc(Display);

    glutSpecialFunc(Special);
    glutSpecialUpFunc(SpecialUp);

    glutIdleFunc(std::bind(&Window::Idle));
    glutEntryFunc(std::bind(&Window::Entry));
    glutKeyboardFunc(std::bind(&Window::Keyboard));

    glutMouseFunc(std::bind(&Window::Mouse, window));
    glutReshapeFunc(std::bind(&Window::Reshape, window));
    glutPassiveMotionFunc(std::bind(&Window::PassiveMotion, window));

    glutMainLoop();

    return 0;
}
