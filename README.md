# sketchpad (Windows 10)

Canvas for testing out the fundamentals of the OpenGL Rendering Pipeline.

## Requirements

- [Premake5](https://github.com/premake/premake-core)
- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) + optional extensions: [GLSL Language Integration](https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL) and [Visual Assist](https://www.wholetomato.com/)
- [GLEW](https://en.wikipedia.org/wiki/OpenGL_Extension_Wrangler_Library) (version 2.1.0)
- [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT) (v3.0.0 MSVC Package), or [GLFW](https://en.wikipedia.org/wiki/GLFW) (version 3.3.2) for finer control

## How to build

- `vendor/premake/premake5.exe vs2019`

## Recommended Visual Studio Settings

The Lua scripts will handle the workspace/project/build settings for all configurations and platforms for us. Besides, it is recommended to consistently use spaces instead of tabs. In case the tab size of the editor and the website rendering the source code do not meet, indentation can be very messy. For example, 4 spaces indentation would become 8 on Github, and there are too many 8 spaces indentation code on Github...

![tab-settings](res/spaces.PNG)

![tab-settings](res/SPC.png)


## Useful References

- [Khronos OpenGL Wiki](https://www.khronos.org/opengl/wiki/Main_Page)
- [OpenGL API Documentation](http://docs.gl/)
- [OpenGL-Tutorials](https://www.opengl-tutorial.org/)
- [Learning Modern 3D Graphics Programming](https://paroj.github.io/gltut/)
- [Anton's OpenGL 4 Tutorials](https://antongerdelan.net/opengl/)
