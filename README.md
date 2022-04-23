# sketchpad

![GitHub license](https://img.shields.io/github/license/neo-mashiro/sketchpad?color=orange&label=License&style=plastic)

This is a simple rendering library built with __OpenGL 4.6__ and C++17, the purpose of which is to experiment with various rendering techniques and see how these ideas are put into practice in a rasterization pipeline. Unlike offline path tracing which is based mostly on math and physics rules, RTR is full of little hacks and compromises due to the 60 FPS tight budget. There are so many low-level details behind the rendering pipeline, so we need a solid understanding of every pixel in graphics. This project is an exercise to learn the basics of graphics in modern OpenGL, also a useful framework and codebase for future reference. I've borrowed some game engine concepts such as the ECS architecture to raise the level of abstraction, so that in the future we can prototype new scenes with relative ease and focus more on the rendering algorithms.

## Demo Video

insert link here

Tested on: GTX 1050 on an old laptop (2016)

## Gallery

<p align="center">
  <b>CONSOLE LOGS</b>
  <br><br>
  <img src="">
</p>

## Dependencies

- [Premake5](https://github.com/premake/premake-core), [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) + optional extensions: [GLSL Language Integration](https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL) and [Visual Assist](https://www.wholetomato.com/)
- [GLEW](https://en.wikipedia.org/wiki/OpenGL_Extension_Wrangler_Library) (version 2.1.0), or better [GLAD](https://glad.dav1d.de/), and [GLM](https://glm.g-truc.net/0.9.2/api/index.html) (version 0.9.2 or above)
- [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT) (v3.0.0 MSVC Package), or [GLFW](https://en.wikipedia.org/wiki/GLFW) (version 3.3.2) for finer control
- [stb](https://github.com/nothings/stb) (image loader), or better [SOIL](https://github.com/littlstar/soil), [SOIL2](https://github.com/SpartanJ/SOIL2) for more image utilities, and [spdlog](https://github.com/gabime/spdlog) (logging library)
- [EnTT](...) for entity-component system.
- [Assimp](https://github.com/assimp/assimp) (use [vcpkg](https://github.com/microsoft/vcpkg) to compile from sources)
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg.exe install assimp              # x86 build
./vcpkg.exe install assimp:x64-windows  # x64 build
```
- [Dear ImGui] HUI
- [xxx] Serialization

## How to build (Windows only)

The Premake Lua scripts will handle the workspace/project/build settings for all configurations and platforms for us, there's no need to set things up in Visual Studio by hand. To build the solution, simply run
```bash
git clone https://github.com/neo-mashiro/sketchpad.git
cd sketchpad/
vendor/premake/premake5.exe vs2019
```
Once you have cloned the repository, the `vendor` folder already contains the pre-compiled binaries of all dependencies listed above, simply open the solution in Visual Studio and fire up the application. The executables are then built into the `bin` folder for each platform (x86 or x64), and all dependent DLLs will be copied over there automatically so you don't need any extra setup.

## Features

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