# sketchpad (Windows 10)

A simple canvas for testing out various topics and rendering techniques in Computer Graphics using the OpenGL Rendering Pipeline.

## Dependencies

- [Premake5](https://github.com/premake/premake-core)
- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) + optional extensions: [GLSL Language Integration](https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL) and [Visual Assist](https://www.wholetomato.com/)
- [GLEW](https://en.wikipedia.org/wiki/OpenGL_Extension_Wrangler_Library) (version 2.1.0), or better [GLAD](https://glad.dav1d.de/), and [GLM](https://glm.g-truc.net/0.9.2/api/index.html) (version 0.9.2 or above)
- [FreeGLUT](https://en.wikipedia.org/wiki/FreeGLUT) (v3.0.0 MSVC Package), or [GLFW](https://en.wikipedia.org/wiki/GLFW) (version 3.3.2) for finer control
- [Assimp](https://github.com/assimp/assimp) (pre-compiled libraries exist but are not cross-platform, use [vcpkg](https://github.com/microsoft/vcpkg) to compile from source code manually)
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg.exe install assimp              # x86 build
./vcpkg.exe install assimp:x64-windows  # x64 build
```

## Visual Studio Settings

The Lua scripts will handle the workspace/project/build settings for all configurations and platforms for us. Besides, it is recommended to consistently use spaces instead of tabs. In case the **tab size** of the editor and the website rendering the source code do not meet, indentation can be very messy. For example, there are so many hard-to-read repos on Github that use inconsistent indentation in the same file, because they are mixing four spaces with two tabs (of size 2) in the IDE, which is then converted to eight spaces on the web...

![tab-settings](res/spaces.PNG)

![tab-settings](res/SPC.png)

## How to build

```bash
git clone https://github.com/neo-mashiro/sketchpad.git
cd sketchpad/
vendor/premake/premake5.exe vs2019
```

## Build a single project

- (WIP) add screenshot: select startup project in the solution's properties window
- (WIP) add screenshot: keyboard controls, message boxes, mouse controls, motions, etc.

## Scenes

- scene1 name (come up with a name for each scene): detailed description (summarize the model, theory and techniques applied in this scene and what to expect)
- scene2 name
- ... 没必要去做load scene的功能，太复杂还影响lua，只要少做几个scene就好了，每个scene内容丰富一点放多个model，把基础的知识点都整合在一起，而不是每个scene都很小只包含一个知识点。这样最后只有几个scene，截图展示也比较容易。

## Useful References

- CMU 15-462/662: [Course Home](http://15462.courses.cs.cmu.edu/fall2020/home), [Video Lectures](https://www.youtube.com/playlist?list=PL9_jI1bdZmz2emSh0UQ5iOdT2xRHFHL7E)
- [Khronos OpenGL Wiki](https://www.khronos.org/opengl/wiki/Main_Page)
- [OpenGL API Documentation](http://docs.gl/)
- [OpenGL-Tutorials](https://www.opengl-tutorial.org/)
- [Learn OpenGL](https://learnopengl.com)
- [OpenGL Video Series, The Cherno](https://www.youtube.com/playlist?list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2)
- [Learning Modern 3D Graphics Programming](https://paroj.github.io/gltut/)
- [Anton's OpenGL 4 Tutorials](https://antongerdelan.net/opengl/)
- [OpenGL Shading Language](https://www.khronos.org/opengl/wiki/OpenGL_Shading_Language)
