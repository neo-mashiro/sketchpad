#include "pch.h"

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>
#include <crtdbg.h>
#include "core/app.h"

#pragma execution_character_set("utf-8")

extern "C" {
    // tell the driver to use a dedicated graphics card (NVIDIA, AMD)
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

using namespace core;

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
    std::cout << "  v(>.<)v Welcome to sketchpad! OpenGL context is now active! (~.^)  \n";
    std::cout << "=====================================================================\n";

    SetConsoleTextAttribute(console, 8);  // light gray text, transparent background

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* System Information" << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- GPU Vendor Name:   " << app.gl_vendor    << std::endl;
    std::cout << "- OpenGL Renderer:   " << app.gl_renderer  << std::endl;
    std::cout << "- OpenGL Version:    " << app.gl_version   << std::endl;
    std::cout << "- GLSL Core Version: " << app.glsl_version << '\n' << std::endl;

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* Maximum supported texture size: " << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- 1D / 2D texture (width and height): " << app.gl_texsize << std::endl;
    std::cout << "- 3D texture (width, height & depth): " << app.gl_texsize_3d << std::endl;
    std::cout << "- Cubemap texture (width and height): " << app.gl_texsize_cubemap << std::endl;
    std::cout << "- Max number of texture image units: "  << app.gl_max_texture_units << '\n' << std::endl;

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* Maximum allowed number of uniform buffers: " << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- Vertex shader:   " << app.gl_maxv_ubos << std::endl;
    std::cout << "- Geometry shader: " << app.gl_maxg_ubos << std::endl;
    std::cout << "- Fragment shader: " << app.gl_maxf_ubos << std::endl;
    std::cout << "- Compute shader:  " << app.gl_maxc_ubos << '\n' << std::endl;

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* Maximum allowed number of shader storage buffers: " << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- Fragment shader: " << app.gl_maxf_ssbos << std::endl;
    std::cout << "- Compute shader:  " << app.gl_maxc_ssbos << '\n' << std::endl;

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "* GPGPU limitation of compute shaders: " << std::endl;
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "- Max number of invocations (threads): " << app.cs_max_invocations << std::endl;
    std::cout << "- Max work group count (x, y, z): " << app.cs_nx << ", " << app.cs_ny << ", " << app.cs_nz << std::endl;
    std::cout << "- Max work group size  (x, y, z): " << app.cs_sx << ", " << app.cs_sy << ", " << app.cs_sz << '\n' << std::endl;

    SetConsoleTextAttribute(console, 15);  // full white text, transparent background

    // from now on, font/color/style of the console text printed by `printf, fprintf, cout, cerr`
    // will be solid white, but those printed by the application core are controlled by spdlog.

    app.Start();  // start the default scene

    // main event loop
    while (true) {
        glutMainLoopEvent();    // resolve all pending freeglut events (scene level), then return to us
        app.PostEventUpdate();  // now we have a chance to do our own update stuff (application level)
    }

    /* if the user requested to exit properly, `app.PostEventUpdate()` will clean up context and data
       first to make sure all stacks are unwound and all destructors are called, and then safely exit,
       so there won't be any memory leaks, and in fact we will never reach here.

       upon exit, the CRT library may still detect and report a few memory leaks in the debug window,
       which are actually just some global static data that lives in the static memory segment, apart
       from the heap and the stack. Static variables are intended to be persistent for the lifetime of
       the application, they are only destructed before the shutdown of the process. Since the program
       is going to terminate anyway, these are not real memory leaks but false positives of the report
    */

    app.Clear();
    return 0;
}
