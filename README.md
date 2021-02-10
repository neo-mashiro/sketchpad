# OpenGL-Canvas (Windows 10)

Canvas for testing out the fundamentals of the OpenGL Rendering Pipeline.

## Requirements

- [Premake5](https://github.com/premake/premake-core)
- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
- [GLEW](https://en.wikipedia.org/wiki/OpenGL_Extension_Wrangler_Library) (version 2.1.0)
- [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT) (v3.0.0 MSVC Package), or [GLFW](https://en.wikipedia.org/wiki/GLFW) (version 3.3.2) for finer control

## How to build

- `vendor/premake/premake5.exe vs2019`
- `[to do] how to selectively build a particular window`

## Recommended Visual Studio Settings

The Lua scripts will handle the workspace/project/build settings for all configurations and platforms for us. Besides, it is recommended to consistently use spaces instead of tabs. In case the tab size of the editor and the website rendering the source code do not meet, indentation can be very messy. For example, 4 spaces indentation would become 8 on Github, and there are too many 8 spaces indentation code on Github...

<img src="res/spaces.PNG" width="745" height="435">

<img src="res/SPC.png" width="180" height="73">


## WIP

- use Lua scripting to automate a build framework (build only a single .cpp and a single windowed app each time, exclude other files from build)

## Reference

- http://docs.gl/
- [Learning Modern 3D Graphics Programming](https://paroj.github.io/gltut/)
