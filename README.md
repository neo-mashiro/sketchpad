# sketchpad - physically-based renderer

![GitHub license](https://img.shields.io/github/license/neo-mashiro/sketchpad?color=orange&label=License&style=plastic)

This is a simple rendering library built with OpenGL 4.6 and C++17, the purpose of which is to experiment with a wide range of rendering techniques and see how these ideas are put into practice in a rasterization pipeline. Unlike offline path tracing which is based mostly on math and physics rules, RTR is full of little hacks and compromises due to the 60 FPS constraint. There are also lots of low-level details behind the graphics API, so we need a solid understanding of every step in the rendering pipeline.

This project is initially started as an exercise to learn the basics of graphics in modern OpenGL, which has since then incorporated some ideas from game engine architecture to raise up the scope and level of abstraction. It is designed with modularization in mind to let users prototype new scenes with relative ease, thus we can focus more on the rendering algorithms without worrying too much about details. This can also be a useful framework and codebase for future reference, and a good starting point for implementing something more feature-complete and advanced.

Watch [Demo Video](https://www.youtube.com/watch?v=JCagITtAmQ0) on Youtube. (Tested on a 2016 old laptop with __NVIDIA GTX 1050__ card)

## Requirements

- [Visual Studio](https://visualstudio.microsoft.com/downloads/) 2019 or later + optional extensions: [GLSL Language Integration](https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL) and [Visual Assist](https://www.wholetomato.com/)
- Desktop Windows with OpenGL 4.6 support + [Premake5](https://github.com/premake/premake-core) (included in the `\vendor` folder)

## Dependencies

- [GLFW](https://en.wikipedia.org/wiki/GLFW) (v3.3.2+) or [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT) (v3.0.0 MSVC Package), [GLAD](https://glad.dav1d.de/), [GLM](https://glm.g-truc.net/0.9.2/api/index.html) (v0.9.2+), [spdlog](https://github.com/gabime/spdlog) (logging library)
- [Dear ImGui](https://github.com/ocornut/imgui) (GUI), [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) (Gizmo), [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders), [Optick](https://github.com/bombomby/optick) (profiler), [taskflow](https://github.com/taskflow/taskflow) (parallel tasks system)
- [EnTT](https://github.com/skypjack/entt) (entity-component system), [Date](https://github.com/HowardHinnant/date) (time zone), [stb](https://github.com/nothings/stb) (image loader), [Assimp](https://github.com/assimp/assimp) (compile from sources)
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg.exe install assimp              # x86 build
./vcpkg.exe install assimp:x64-windows  # x64 build
```

## How to build (Windows only)

The `premake5.lua` script will handle all the workspace/project/build settings for us, there's no need to configure `build/release` or `win32/x64` manually, just run `premake.bat` and you are good to go. Alternatively, you can also build the solution on the command line like so:
```bash
git clone https://github.com/neo-mashiro/sketchpad.git
cd sketchpad/
vendor/premake/premake5.exe vs2019
```
The `\vendor` folder already contains the pre-compiled binaries of all dependencies listed above, simply open the solution in Visual Studio and start building. Upon success, executables will be built into a sub-folder in `\bin` for the selected platform, and all dependent DLLs will be automatically copied over there. You can also move the exe folder around without problems, as long as it's inside root, paths are automatically deducted.

## Screenshots

<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/11.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/22.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/33.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/44.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/55.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/66.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/77.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/88.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/99.png"></p>
<p align="center"><img src="https://raw.githubusercontent.com/neo-mashiro/sketchpad/main/res/screenshot/00.png"></p>

![image](/res/screenshot/console.png?raw=true "debug console")

## Features

| Application                                     | Rendering                                           |
| ----------------------------------------------- | --------------------------------------------------- |
| manage objects with entity-component system     | tiled forward rendering (screen space)              |
| runtime scene loading and scene switching       | physically-based shading and image-based lighting   |
| skeleton animation for humanoid models          | physically-based materials (simplified Disney BSDF) |
| FPS camera with smooth zoom and arcball control | compute shader IBL baking                           |
| blazingly fast native screen capturing (GDI+)   | compute shader bloom effect                         |
| independent resource and asset managers         | compute shader cloth simulation                     |
| smart material system and viewable framebuffers | configurable wireframe rendering                    |
| smart OpenGL context switching                  | Blender-style infinite grid                         |
| automatic resolve `std140` layout               | off-screen MSAA                                     |
| support `#include` directives in GLSL           | omnidirectional PCSS shadows (Poisson disk)         |
| all shaders in one file (except compute shader) | runtime transformation control using Gizmos         |
| built-in clock and framerate counter            | tone mapping and gamma correction                   |

## TODO list?

- Fix cloth blending artifacts in scene04 using Order-Independent Transparency (OIT)
- Data-oriented scene graph, transformation trees, see "3D Graphics Rendering Cookbook 2021" Ch.7
- Scene graph serialization in readable YAML format
- Precomputed Radiance Transfer (PRT) and light transport, see GAMES202 homework 2
- Screen Space Reflection (SSR), see GAMES202 homework 3
- Frame Graph (reference Filament) and Temporal Anti-Aliasing (TAA)
- Raymarching, SDFs and tessellated terrain, see https://iquilezles.org/articles/
- Compute shader particle systems, realistic fire, smoke and water simulation
- Volumetric lights and subsurface scattering (can only hack in realtime, not physically based)
- Optimization: frustum culling, clustered tiled rendering (voxelized), batch rendering, indexed drawing
- Refactor design to include Taskflow and Optics, integrate [Bullet](https://github.com/bulletphysics/bullet3) physics and [OpenAL](https://www.openal.org/) audio

## References

- [GLSL 4.60 Specification](https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.4.60.html)
- [Learn OpenGL Tutorial](https://learnopengl.com)
- [C++ & Hazel Game Engine Series](https://www.youtube.com/channel/UCQ-W1KE9EYfdxhL6S4twUNw) by Cherno
- [OpenGL 4 Shading Language Cookbook, Third Edition](https://www.packtpub.com/product/opengl-4-shading-language-cookbook-third-edition/9781789342253)
- [Physically Based Rendering in Filament](https://google.github.io/filament/Filament.html)
- [SIGGRAPH Physically Based Shading Course 2012-2020](https://blog.selfshadow.com/publications/)
- [pbrt book v3](https://www.pbr-book.org/)
- [GAMES 202, Advanced Real-time Rendering](https://sites.cs.ucsb.edu/~lingqi/teaching/games202.html)
- [CMU 15-462, Compute Graphics](http://15462.courses.cs.cmu.edu/fall2020/home)
- [CMU 15-468, Physics-based Rendering](http://graphics.cs.cmu.edu/courses/15-468/)
