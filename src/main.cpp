#include "pch.h"

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#include "core/app.h"

#pragma execution_character_set("utf-8")

extern "C" {
    // tell the driver to use a dedicated graphics card (NVIDIA, AMD)
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

int main(int argc, char** argv) {
    // set console code page to unicode (UTF-8)
    SetConsoleOutputCP(65001);

    // set unicode console title
    SetConsoleTitle((LPCWSTR)(L"Sketchpad Console"));

    // set console window position and size
    HWND window = GetConsoleWindow();
    SetWindowPos(window, 0, 0, 0, 1024, 768, SWP_NOZORDER);

    // enable memory-leak report (MSVC intrinsic)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // initialize application
    auto& app = core::Application::GetInstance();
    app.Init(argc, argv);

    // print context information
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(console, FOREGROUND_INTENSITY | FOREGROUND_BLUE);

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "$ (#^_^#) Welcome to sketchpad! OpenGL context is now active! (~.^) $\n";
    std::cout << "---------------------------------------------------------------------\n" << std::endl;

    SetConsoleTextAttribute(console, FOREGROUND_INTENSITY);

    std::cout << "$ System Information" << '\n' << std::endl;
    std::cout << "  GPU Vendor Name:   " << app.gl_vendor    << std::endl;
    std::cout << "  OpenGL Renderer:   " << app.gl_renderer  << std::endl;
    std::cout << "  OpenGL Version:    " << app.gl_version   << std::endl;
    std::cout << "  GLSL Core Version: " << app.glsl_version << '\n' << std::endl;

    std::cout << "$ Maximum supported texture size: " << '\n' << std::endl;
    std::cout << "  1D / 2D texture (width and height): " << app.gl_texsize           << std::endl;
    std::cout << "  3D texture (width, height & depth): " << app.gl_texsize_3d        << std::endl;
    std::cout << "  Cubemap texture (width and height): " << app.gl_texsize_cubemap   << std::endl;
    std::cout << "  Max number of image units: "          << app.gl_max_image_units   << std::endl;
    std::cout << "  Max number of texture units: "        << app.gl_max_texture_units << '\n' << std::endl;

    std::cout << "$ Maximum allowed number of uniform buffers: " << '\n' << std::endl;
    std::cout << "  Vertex shader:   " << app.gl_maxv_ubos << std::endl;
    std::cout << "  Geometry shader: " << app.gl_maxg_ubos << std::endl;
    std::cout << "  Fragment shader: " << app.gl_maxf_ubos << std::endl;
    std::cout << "  Compute shader:  " << app.gl_maxc_ubos << '\n' << std::endl;

    std::cout << "$ Maximum allowed number of shader storage buffers: " << '\n' << std::endl;
    std::cout << "  Fragment shader: " << app.gl_maxf_ssbos << std::endl;
    std::cout << "  Compute shader:  " << app.gl_maxc_ssbos << '\n' << std::endl;

    std::cout << "$ GPGPU limitation of compute shaders: " << '\n' << std::endl;
    std::cout << "  Max number of invocations (threads): " << app.cs_max_invocations << std::endl;
    std::cout << "  Max work group count (x, y, z): " << app.cs_nx << ", " << app.cs_ny << ", " << app.cs_nz << std::endl;
    std::cout << "  Max work group size  (x, y, z): " << app.cs_sx << ", " << app.cs_sy << ", " << app.cs_sz << std::endl;
    std::cout << '\n' << std::endl;

    SetConsoleTextAttribute(console, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    // from now on, font/color/style of the console text printed by `printf, fprintf, cout, cerr`
    // will be solid white, but those printed by the application core are controlled by "spdlog".

    app.Start();  // start the welcome screen

    // main event loop
    while (true) {
        app.MainEventUpdate();  // resolve all pending draw events (scene level) and then return to us
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
