# "sketchpad"

![GitHub license](https://img.shields.io/github/license/neo-mashiro/sketchpad?color=orange&label=License&style=plastic)

This is a simple rendering library built with OpenGL 4.6 and C++17, the purpose of which is to experiment with a wide range of rendering techniques and see how these ideas are put into practice in a rasterization pipeline. Unlike offline path tracing which is based mostly on math and physics rules, RTR is full of little hacks and compromises due to the 60 FPS constraint. There are also lots of low-level details behind the graphics API, so we must have a solid understanding of every little step in the rendering pipeline.

This project is initially started as an exercise to learn the basics of graphics in modern OpenGL, which later incorporated some ideas from game engine architecture to raise up the scope and level of abstraction. It is designed with modularization in mind to let users prototype new scenes with relative ease, thus we can focus more on the rendering algorithms without worrying too much about details. This can also be a useful framework and codebase for future reference, and a good starting point for implementing something more feature-complete and advanced.

[Watch Demo Video](insert link here) on Youtube. (Tested on a 2016 old laptop with NVIDIA GTX 1050 card)

## Requirements

- [Visual Studio](https://visualstudio.microsoft.com/downloads/) 2019 or later + optional extensions: [GLSL Language Integration](https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL) and [Visual Assist](https://www.wholetomato.com/)
- Desktop Windows with OpenGL 4.6 support + [Premake5](https://github.com/premake/premake-core) (included in the `\vendor` folder)

## Dependencies

- [GLFW](https://en.wikipedia.org/wiki/GLFW) (v3.3.2 and above) or [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT) (v3.0.0 MSVC Package), [GLAD](https://glad.dav1d.de/), [GLM](https://glm.g-truc.net/0.9.2/api/index.html) (v0.9.2 and above), [spdlog](https://github.com/gabime/spdlog) (logging library)
- [Dear ImGui](https://github.com/ocornut/imgui) (GUI), [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) (Gizmo), [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) (UI Icons), [Optick](https://github.com/bombomby/optick) (profiler), [taskflow](https://github.com/taskflow/taskflow) (parallel tasks system)
- [EnTT](https://github.com/skypjack/entt) (entity-component system), [Date](https://github.com/HowardHinnant/date) (time zone), [stb](https://github.com/nothings/stb) (image loader), [Assimp](https://github.com/assimp/assimp) (use [vcpkg](https://github.com/microsoft/vcpkg) to compile from sources)
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg.exe install assimp              # x86 build
./vcpkg.exe install assimp:x64-windows  # x64 build
```

## Build (Windows only)

The `premake5.lua` script will handle all the workspace/project/build settings for us, there's no need to configure `build/release` or `win32/x64` manually, just run `premake.bat` and you are good to go. Alternatively, you can build the solution in console using
```bash
git clone https://github.com/neo-mashiro/sketchpad.git
cd sketchpad/
vendor/premake/premake5.exe vs2019
```
The `\vendor` folder already contains the pre-compiled binaries of all dependencies listed above, simply open the solution in Visual Studio and start building. Upon success, executables will be built into a sub-folder in `\bin` for the selected platform, and all dependent DLLs will be automatically copied over there. You can also move the exe folder around, as long as it's inside root, paths are automatically deducted.

## Screenshots

<p align="center">
  <b>CONSOLE LOGS</b>
  <br><br>
  <img src="">
</p>

## Features

make a compact table here

- managing objects with entity-component system
- loading and switching scenes at runtime
- loading external 3D models, motions and high resolution HDRI
- skeleton animation for humanoid models
- blazingly fast screen capture support
- sharing assets between entities or scenes
- automatic PBR material system (like materials in Unity)
- smart asset bindings (uniforms, shaders, textures and vertex arrays)
- building UBOs automatically from shaders (require the `std140` layout)
- configuring and visualizing framebuffers with one call
- all shader stages in a one file (except the compute shader)
- saving and loading shader binaries (GLSL only, SPIR-V not supported yet)
- robust debugging tools (detailed console logs + debug callback + debugging by checkpoints)
- modularized shader system (support C++ style `#include` directives in GLSL files)
- dynamic transformation control using Gizmos
- first-person camera + arcball control + smooth zooming and recovery
- built-in clock and framerate counter
- automatic filepath deduction (can run `.exe` in any folder)
- mapping between 3D vector, equirectangular uv coordinates and ILS vector
- tiled forward rendering (light culling in compute shader)
- physically-based shading and IBL (with energy compensation)
- physically-based materials (Disney BRDF + clear coat + anisotropy + refraction + cloth)
- compute shader based IBL precomputation (with multi-scattering)
- compute shader based bloom (1 downsample + Gaussian blur + 1 upsample)
- compute shader based cloth simulation
- configurable wireframe rendering
- rendering skybox with custom exposure and LOD level
- rendering infinite grid (Blender style)
- rendering icons and rotating characters in ImGui
- off-screen multisample anti-aliasing (MSAA)
- omnidirectional shadow mapping (PCSS with Poisson disk sampling)
- directional lights, point lights and spotlights
- tone mapping and gamma correction

## TODO list?

- Data-oriented scene graph, transformation trees, see "3D Graphics Rendering Cookbook 2021" Ch.7
- Scene graph serialization in readable YAML format
- Precomputed Radiance Transfer (PRT) and light transport, see GAMES202 homework 2
- Screen Space Reflection (SSR), see GAMES202 homework 3
- Frame Graph (reference Filament) and Temporal Anti-Aliasing (TAA)
- Raymarching, SDFs, tessellated terrain, compute shader based particle systems
- Volumetric lights and subsurface scattering (can only hack in realtime, not physically based)
- Optimization: frustum culling, clustered tiled forward rendering, batch rendering, indexed drawing
- Revise design to include Taskflow and integrate Bullet physics
- Realtime ray tracing and denoiser, see GAMES202 homework 5



## References

- https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.4.60.html

- [Physically Based Rendering: From Theory To Implementation](https://www.pbr-book.org/)
- [Learn OpenGL](https://learnopengl.com)
- [Hazel Game Engine Series, The Cherno](....)
- [OpenGL 4 Shading Language Cookbook, Third Edition](...)
- GAMES202 - [Advanced Real-time Rendering](https://sites.cs.ucsb.edu/~lingqi/teaching/games202.html)
- CMU 15-462 - [course home](http://15462.courses.cs.cmu.edu/fall2020/home), [video lectures](https://www.youtube.com/playlist?list=PL9_jI1bdZmz2emSh0UQ5iOdT2xRHFHL7E)
- [Khronos OpenGL Wiki](https://www.khronos.org/opengl/wiki/Main_Page)

complete else from scripts
