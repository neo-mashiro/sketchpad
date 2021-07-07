#pragma once

#include <windows.h>
#include <GL/glew.h>

#pragma warning(push)
#pragma warning(disable : 4505)
#include <GL/freeglut.h>
#pragma warning(pop)

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/perpendicular.hpp>
#pragma warning(pop)

#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl3.h"


namespace Sketchpad {

    enum class WindowLayer { scene, imgui, win32 };

    struct Window {
        int id;
        const char* title;
        GLuint width, height;
        GLuint full_width, full_height;
        float aspect_ratio;
        int zoom, pos_x, pos_y;
        GLuint display_mode;
        WindowLayer current_layer;
    };

    struct FrameCounter {
        float last_frame, this_frame, delta_time, time;
    };

    struct MouseState {
        GLuint pos_x, pos_y;
        int delta_x, delta_y;
    };

    struct KeyState {
        bool u, d;        // up and down
        bool f, b, l, r;  // forward, backward, left, right
    };

    // canvas can be treated as a sealed singleton instance that survives the entire application lifecycle

    class Canvas final {
      private:
        Canvas();
        ~Canvas() {};

      public:
        struct Window window;
        struct FrameCounter frame_counter;
        struct MouseState mouse;
        struct KeyState keystate;

        bool opengl_context_active;
        std::string gl_vendor, gl_renderer, gl_version, glsl_version;
        int gl_texsize, gl_texsize_3d, gl_texsize_cubemap, gl_max_texture_units;

        Canvas(Canvas& canvas) = delete;         // delete copy constructor
        void operator=(const Canvas&) = delete;  // delete assignment operator

        static Canvas* GetInstance();

        // initialize ImGui backends and setup options
        static void CreateImGuiContext();

        // check if a valid OpenGL context is present, used by other modules to validate context
        static void CheckOpenGLContext(const std::string& call);

        // keep track of the frame statistics, all scene updates depend on this
        static void Update();

        // clean up the canvas
        static void Clear();

        // switch to another scene
        static void SwitchScene();

        // default event callbacks
        static void Idle(void);
        static void Entry(int state);
        static void Keyboard(unsigned char key, int x, int y);
        static void KeyboardUp(unsigned char key, int x, int y);
        static void Reshape(int width, int height);
        static void Motion(int x, int y);
        static void PassiveMotion(int x, int y);
        static void Mouse(int button, int state, int x, int y);
        static void Special(int key, int x, int y);
        static void SpecialUp(int key, int x, int y);
    };
}
