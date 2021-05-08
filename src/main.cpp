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

void InitScene(void);

void Display(void);

int main(int argc, char** argv) {
    SetConsoleOutputCP(65001);  // set the console code page to utf-8

    glutInit(&argc, argv);

    Canvas* canvas = Canvas::GetInstance();
    Window* winptr = &(canvas->window);

    glutInitDisplayMode(winptr->display_mode);
    glutInitWindowSize(winptr->width, winptr->height);
    glutInitWindowPosition(winptr->pos_x, winptr->pos_y);

    winptr->id = glutCreateWindow(winptr->title);

    if (winptr->id <= 0) {
        std::cerr << "Unable to create a window..." << std::endl;
        exit(EXIT_FAILURE);
    }

    glutSetCursor(GLUT_CURSOR_INHERIT);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Unable to initialize GLEW..." << std::endl;
        exit(EXIT_FAILURE);
    }

    canvas->opengl_context_active = true;

    std::cout << "===================================================================" << std::endl;
    std::cout << "v(^_^)v Welcome to sketchpad! OpenGL context is now active! m(~_^)m" << std::endl;
    std::cout << "===================================================================" << std::endl;

    canvas->gl_vendor    = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    canvas->gl_renderer  = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    canvas->gl_version   = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    canvas->glsl_version = std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

    std::cout << "Graphics card vendor: "   << canvas->gl_vendor    << std::endl;
    std::cout << "OpenGL renderer: "        << canvas->gl_renderer  << std::endl;
    std::cout << "OpenGL current version: " << canvas->gl_version   << std::endl;
    std::cout << "GLSL primary version: "   << canvas->glsl_version << '\n' << std::endl;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(canvas->gl_texsize));
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &(canvas->gl_texsize_3d));
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &(canvas->gl_texsize_cubemap));

    std::cout << "Maximum texture size the GPU supports: " << std::endl;
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "1D/2D texture (width and height): "   << canvas->gl_texsize << std::endl;
    std::cout << "3D texture (width, height & depth): " << canvas->gl_texsize_3d << std::endl;
    std::cout << "Cubemap texture (width and height): " << canvas->gl_texsize_cubemap << '\n' << std::endl;

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &(canvas->gl_max_texture_units));
    std::cout << "Maximum number of samplers supported in the fragment shader: " << canvas->gl_max_texture_units << '\n' << std::endl;

    // register debug callback
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);

    InitScene();

    glutDisplayFunc(Display);

    glutIdleFunc(Canvas::Idle);
    glutEntryFunc(Canvas::Entry);
    glutKeyboardFunc(Canvas::Keyboard);
    glutKeyboardUpFunc(Canvas::KeyboardUp);
    glutMouseFunc(Canvas::Mouse);
    glutReshapeFunc(Canvas::Reshape);
    glutPassiveMotionFunc(Canvas::PassiveMotion);
    glutSpecialFunc(Canvas::Special);
    glutSpecialUpFunc(Canvas::SpecialUp);

    glutMainLoop();

    return 0;
}
