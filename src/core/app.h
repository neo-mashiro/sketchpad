#pragma once

#include <string>

namespace core {

    class Application final {
      private:
        bool opengl_context_active = false;

        Application() {}
        ~Application() {};

      public:
        std::string gl_vendor, gl_renderer, gl_version, glsl_version;
        GLint gl_texsize, gl_texsize_3d, gl_texsize_cubemap, gl_max_texture_units;
        GLint gl_maxv_ubos, gl_maxg_ubos, gl_maxf_ubos, gl_maxc_ubos;
        GLint gl_maxf_ssbos, gl_maxc_ssbos;
        GLint cs_nx, cs_ny, cs_nz, cs_sx, cs_sy, cs_sz, cs_max_invocations;

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
