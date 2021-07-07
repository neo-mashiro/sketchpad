#include "canvas.h"
#include "log.h"

#pragma execution_character_set("utf-8")

#pragma warning(push)
#pragma warning(disable : 4100)

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
    #pragma warning(pop)
}

void Start(void);

void Update(void);

int main(int argc, char** argv) {
    SetConsoleOutputCP(65001);  // set the console code page to utf-8

    glutInit(&argc, argv);

    using namespace Sketchpad;
    Log::Init();

    Canvas* canvas = Canvas::GetInstance();
    Window* winptr = &(canvas->window);

    glutInitDisplayMode(winptr->display_mode);
    glutInitWindowSize(winptr->width, winptr->height);
    glutInitWindowPosition(winptr->pos_x, winptr->pos_y);

    winptr->id = glutCreateWindow(winptr->title);

    if (winptr->id <= 0) {
        CORE_ERROR("Unable to create a window...");
        exit(EXIT_FAILURE);
    }

    if (glewInit() != GLEW_OK) {
        CORE_ERROR("Unable to initialize GLEW...");
        exit(EXIT_FAILURE);
    }

    canvas->CreateImGuiContext();
    canvas->opengl_context_active = true;

    glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(console, 3);   // water blue text, transparent background

    std::cout << "=====================================================================\n";
    std::cout << "| v(^_^)v Welcome to sketchpad! OpenGL context is now active! (~.^) |\n";
    std::cout << "=====================================================================\n";

    SetConsoleTextAttribute(console, 8);   // light gray text, transparent background

    canvas->gl_vendor    = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    canvas->gl_renderer  = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    canvas->gl_version   = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    canvas->glsl_version = std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* System Information" << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- GPU Vendor Name:   " << canvas->gl_vendor    << std::endl;
    std::cout << "- OpenGL Renderer:   " << canvas->gl_renderer  << std::endl;
    std::cout << "- OpenGL Version:    " << canvas->gl_version   << std::endl;
    std::cout << "- GLSL Core Version: " << canvas->glsl_version << '\n' << std::endl;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(canvas->gl_texsize));
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &(canvas->gl_texsize_3d));
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &(canvas->gl_texsize_cubemap));

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* Maximum GPU-supported texture size: " << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- 1D / 2D texture (width and height): " << canvas->gl_texsize << std::endl;
    std::cout << "- 3D texture (width, height & depth): " << canvas->gl_texsize_3d << std::endl;
    std::cout << "- Cubemap texture (width and height): " << canvas->gl_texsize_cubemap << std::endl;

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &(canvas->gl_max_texture_units));
    std::cout << "- Maximum number of samplers supported in the fragment shader: " <<
        canvas->gl_max_texture_units << '\n' << std::endl;

    SetConsoleTextAttribute(console, 15);  // full white text, transparent background

    // register debug callback
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);

    glutIdleFunc(Canvas::Idle);
    glutEntryFunc(Canvas::Entry);
    glutKeyboardFunc(Canvas::Keyboard);
    glutKeyboardUpFunc(Canvas::KeyboardUp);
    glutMouseFunc(Canvas::Mouse);
    glutReshapeFunc(Canvas::Reshape);
    glutMotionFunc(Canvas::Motion);
    glutPassiveMotionFunc(Canvas::PassiveMotion);
    glutSpecialFunc(Canvas::Special);
    glutSpecialUpFunc(Canvas::SpecialUp);

    Start();

    glutDisplayFunc(Update);

    glutMainLoop();

    canvas->Clear();

    return 0;
}
