# OpenGL on Windows

## Clean Environment Setup

1. Create an empty project in Visual Studio 2019 or later, copy the folders to your project folder (or simply clone this repository).
2. Go to "Project > Properties", select "All Configurations".
3. Under "General", configure the "Output Directory" as `$(SolutionDir)bin\$(Platform)\$(Configuration)\`.
4. Under "General", configure the "Intermediate Directory" as `$(SolutionDir)bin\intermediates\$(Platform)\$(Configuration)\`.
5. Under "C/C++ > General", configure the "Additional Include Directories" as `$(SolutionDir)dependencies\freeGLUT\include;$(SolutionDir)dependencies\GLFW\include;$(SolutionDir)dependencies\GLEW\include`.
6. Under "C/C++ > Precompiled Headers", configure the "Precompiled Header" option to "Not Using Precompiled Headers". This is for safety reasons as some source files in the dependencies may not follow the pch model unless modified.
6. Under "C/C++ > Preprocessor", add `GLEW_STATIC` to the "Preprocessor Definitions".
7. Under "Linker > General", configure the "Additional Library Directories" as `$(SolutionDir)dependencies\freeGLUT\lib;$(SolutionDir)dependencies\GLFW\lib-vc2019;$(SolutionDir)dependencies\GLEW\lib\Release\Win32`.
7. Under "Linker > Input", add `glew32s.lib` to the "Additional Dependencies".
8. Under "Linker > Advanced", configure the "Entry Point" as `mainCRTStartup`.
9. Copy the corresponding DLLs of dependencies to the `$(SolutionDir)` folder and ship them with the application. Or, copy them to the system folder if that's convenient for you. (x64 -> `C:\Windows\SysWOW64`, x86 -> `C:\Windows\System32`)

## Dependencies

- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
- [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT)(v3.0.0 MSVC Package) or [GLFW](https://en.wikipedia.org/wiki/GLFW)(version 3.3.2) for finer control
- [GLEW](https://en.wikipedia.org/wiki/OpenGL_Extension_Wrangler_Library)

For setup on Linux, may use the cross-platform IDE **Clion** and install libraries from the terminal.

```bash
sudo apt-get install libglfw3
sudo apt-get install freeglut3 freeglut3-dev
sudo apt-get install libglew1.5-dev libglm-dev
...
```

## Reference

- http://docs.gl/
