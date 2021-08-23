#pragma once

#include <string>

namespace core {

    class Application final {
      private:
        Application() {};
        ~Application() {};

      public:
        // hardware information
        std::string gl_vendor, gl_renderer, gl_version, glsl_version;
        int gl_texsize, gl_texsize_3d, gl_texsize_cubemap, gl_max_texture_units;
        int gl_n_msaa_buffers, gl_msaa_buffer_size;

        bool opengl_context_active = false;

        Application(Application& application) = delete;
        void operator=(const Application&) = delete;

        static Application& GetInstance();
        static bool IsContextActive();

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
        void Clear(void);
    };
}
