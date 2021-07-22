#pragma once

#include <string>
#include "scene/scene.h"

using namespace scene;

// whenever possible, place your #includes in the cpp file instead of header, this helps
// reduce compilation time and size of other translation units that include this header.

namespace core {

    // despite the numerous drawbacks and pitfalls of using the singleton pattern, our
    // application class is a really nice example of where singletons can be useful.
    // our global application instance survives the entire program lifecycle, it's just
    // a bunch of global state variables and functions grouped into an organized block.

    class Application final {
      private:
        Application() {};
        ~Application() {};

      public:
        // hardware information
        std::string gl_vendor, gl_renderer, gl_version, glsl_version;
        int gl_texsize, gl_texsize_3d, gl_texsize_cubemap, gl_max_texture_units;

        bool opengl_context_active = false;
        Scene* last_scene = nullptr;
        Scene* active_scene = nullptr;

        static Application& GetInstance();               // singleton instance accessor
        Application(Application& application) = delete;  // delete copy constructor
        void operator=(const Application&) = delete;     // delete assignment operator

        // check if a valid OpenGL context is present, used by other modules to validate context
        void CheckOpenGLContext(const std::string& call) const;

        // freeglut event callbacks
        static void Idle(void);
        static void Display(void);
        static void Entry(int state);
        static void Keyboard(unsigned char key, int x, int y);
        static void KeyboardUp(unsigned char key, int x, int y);
        static void Reshape(int width, int height);
        static void Motion(int x, int y);
        static void PassiveMotion(int x, int y);
        static void Mouse(int button, int state, int x, int y);
        static void Special(int key, int x, int y);
        static void SpecialUp(int key, int x, int y);

        // core event functions
        void Init(void);
        void Start(void);
        void PostEventUpdate(void);
        void Switch(const std::string& title);
        void Clear(void);
    };
}
