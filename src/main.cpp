#include "pch.h"

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>

#include "core/app.h"
#include "core/log.h"
#include "scene/scene.h"

#pragma execution_character_set("utf-8")

using namespace core;

extern "C" {
    // tell the driver to use a dedicated graphics card (NVIDIA, AMD)
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

int main(int argc, char** argv) {
    // set the console code page to utf-8
    SetConsoleOutputCP(65001);

    // enable memory-leak report (MSVC intrinsic)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    glutInit(&argc, argv);

    auto& app = Application::GetInstance();
    app.Init();

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(console, 3);  // water blue text, transparent background

    std::cout << "=====================================================================\n";
    std::cout << "  v(^_^)v Welcome to sketchpad! OpenGL context is now active! (~.^)  \n";
    std::cout << "=====================================================================\n";

    SetConsoleTextAttribute(console, 8);  // light gray text, transparent background

    app.gl_vendor    = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    app.gl_renderer  = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    app.gl_version   = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    app.glsl_version = std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* System Information" << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- GPU Vendor Name:   " << app.gl_vendor    << std::endl;
    std::cout << "- OpenGL Renderer:   " << app.gl_renderer  << std::endl;
    std::cout << "- OpenGL Version:    " << app.gl_version   << std::endl;
    std::cout << "- GLSL Core Version: " << app.glsl_version << '\n' << std::endl;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(app.gl_texsize));
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &(app.gl_texsize_3d));
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &(app.gl_texsize_cubemap));
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &(app.gl_max_texture_units));

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* Maximum supported texture size: " << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- 1D / 2D texture (width and height): " << app.gl_texsize << std::endl;
    std::cout << "- 3D texture (width, height & depth): " << app.gl_texsize_3d << std::endl;
    std::cout << "- Cubemap texture (width and height): " << app.gl_texsize_cubemap << std::endl;
    std::cout << "- Maximum number of samplers supported in the fragment shader: " <<
        app.gl_max_texture_units << '\n' << std::endl;

    glGetIntegerv(GL_SAMPLES, &(app.gl_msaa_buffer_size));
    glGetIntegerv(GL_SAMPLE_BUFFERS, &(app.gl_n_msaa_buffers));

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* Multisample anti-aliasing: " << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- MSAA buffers available: " << app.gl_n_msaa_buffers << std::endl;
    std::cout << "- MSAA buffer size (samples per pixel): " << app.gl_msaa_buffer_size << '\n' << std::endl;

    SetConsoleTextAttribute(console, 15);  // full white text, transparent background

    // from now on, font/color/style of the console text printed by `printf, fprintf, cout, cerr`
    // will be solid white, but those printed by the application core are controlled by spdlog.

    app.Start();  // start the default scene

    // main event loop
    while (true) {
        glutMainLoopEvent();    // resolve all pending freeglut events (scene level), then return to us
        app.PostEventUpdate();  // now we have a chance to do our own update stuff (application level)
    }

    // if the user requested to exit properly, `app.PostEventUpdate()` will clean up context and data
    // first to make sure all stacks are unwound and all destructors are called, and then safely exit,
    // so there won't be any memory leaks, and in fact we will never reach here.

    // upon exit, the CRT library may still detect and report a few memory leaks in the debug window,
    // which are actually just some global static data that lives in the static memory segment, apart
    // from the heap and the stack. Static variables are intended to be persistent for the lifetime of
    // the application, they are only destructed before the shutdown of the process. Since the program
    // is going to terminate anyway, these are not real memory leaks but false positives of the report

    app.Clear();
    return 0;
}
