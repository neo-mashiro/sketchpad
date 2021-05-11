# sketchpad (Windows 10)

![GitHub license](https://img.shields.io/github/license/neo-mashiro/sketchpad?color=orange&label=License&style=plastic)

A simple canvas for testing out various topics and rendering techniques in computer graphics using the OpenGL rendering pipeline. This framework was built to quickly test some low-level graphics features and to learn how they work in OpenGL. Future versions may consider using the [SPIR-V](https://www.khronos.org/spir/) open source ecosystem to support Vulkan or OpenCL.

从零开始手写的一个OpenGL练习框架，用于实现GAMES202中的作业内容和其他一些有趣的东西。

## Dependencies

- [Premake5](https://github.com/premake/premake-core), [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) + optional extensions: [GLSL Language Integration](https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL) and [Visual Assist](https://www.wholetomato.com/)
- [GLEW](https://en.wikipedia.org/wiki/OpenGL_Extension_Wrangler_Library) (version 2.1.0), or better [GLAD](https://glad.dav1d.de/), and [GLM](https://glm.g-truc.net/0.9.2/api/index.html) (version 0.9.2 or above)
- [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT) (v3.0.0 MSVC Package), or [GLFW](https://en.wikipedia.org/wiki/GLFW) (version 3.3.2) for finer control
- [stb](https://github.com/nothings/stb) (image loader), or better [SOIL](https://github.com/littlstar/soil), [SOIL2](https://github.com/SpartanJ/SOIL2) for more image utilities, and [spdlog](https://github.com/gabime/spdlog) (logging library)
- [Assimp](https://github.com/assimp/assimp) (pre-compiled libraries not cross-platform, use [vcpkg](https://github.com/microsoft/vcpkg) to compile from source code)
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg.exe install assimp              # x86 build
./vcpkg.exe install assimp:x64-windows  # x64 build
```

## How to build

The Premake Lua scripts will handle the workspace/project/build settings for all configurations and platforms for us, there's no need to set things up in Visual Studio by hand. To build the solution, simply run
```bash
git clone https://github.com/neo-mashiro/sketchpad.git
cd sketchpad/
vendor/premake/premake5.exe vs2019
```
Once the solution is built, it contains multiple projects in Visual Studio, each one has its own `scene.cpp` and shader files (**client**) that build a separate scene, but all projects share the same code base directly under the `src` folder (**core**). The framework was designed such that only minimal amount of work is needed in `scene.cpp` to build a custom scene, simply select a startup project in the solution's properties window and fire up the application. The executables will be built into the `bin` folder, all dependent DLLs are automatically copied over there.

As an aside, it is recommended to use spaces everywhere instead of tabs. In case the **tab size** of the editor and the website rendering the source code do not meet, indentation can be very messy. For example, there are so many hard-to-read repos on Github that use inconsistent indentation in the same file, because they are mixing four spaces with two tabs (of size 2) in the IDE, which is then converted to eight spaces on Github...

## Mouse and keyboard control

- on application startup, cursor is locked into the window and set to invisible.
- move the mouse to look around the scene from a first-person perspective.
- scroll up/down the mouse wheel to zoom in or zoom out the main camera.
- use directional keys or `wasd` to walk around the scene, use `space` and `z` to go up and down.
- press `enter` to open the menu (cursor enabled), where you can control GUI buttons, slidebars, etc.
- press `esc` to pop up the exit message box, click ok to confirm exit or cancel to resume.

## Sample scenes

#### Console Logs
![image](media/console.png)

#### 3D Meshes, Transformations, Texture Mapping
...

#### Skybox, Reflection, Blinn-Phong Shading
...

#### Custom Model, Lighting Maps & Casters
...



## Useful References

- CMU 15-462 - [course home](http://15462.courses.cs.cmu.edu/fall2020/home), [video lectures](https://www.youtube.com/playlist?list=PL9_jI1bdZmz2emSh0UQ5iOdT2xRHFHL7E)
- GAMES202 - [Advanced Real-time Rendering](https://sites.cs.ucsb.edu/~lingqi/teaching/games202.html)
- [Khronos OpenGL Wiki](https://www.khronos.org/opengl/wiki/Main_Page)
- [OpenGL API Documentation](http://docs.gl/)
- [Learn OpenGL](https://learnopengl.com)
- [OpenGL Video Series, The Cherno](https://www.youtube.com/playlist?list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2)
- [Learning Modern 3D Graphics Programming](https://paroj.github.io/gltut/)
- [Anton's OpenGL 4 Tutorials](https://antongerdelan.net/opengl/)
- [GLSL v4.60 Specification](https://github.com/neo-mashiro/sketchpad/blob/main/res/GLSL%20v4.60%20Spec.pdf)
