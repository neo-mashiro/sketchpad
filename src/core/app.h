#pragma once

#include <string>

namespace core {

    class Application final {
      private:
        bool app_pause = false;
        bool app_shutdown = false;
        bool gl_context_active = false;

      public:
        std::string gl_vendor, gl_renderer, gl_version, glsl_version;
        GLint gl_texsize, gl_texsize_3d, gl_texsize_cubemap;
        GLint gl_max_texture_units, gl_max_image_units;
        GLint gl_max_color_buffs;
        GLint gl_maxv_atcs, gl_maxf_atcs, gl_maxc_atcs;
        GLint gl_maxv_ubos, gl_maxg_ubos, gl_maxf_ubos, gl_maxc_ubos;
        GLint gl_maxf_ssbos, gl_maxc_ssbos;
        GLint cs_nx, cs_ny, cs_nz, cs_sx, cs_sy, cs_sz, cs_max_invocations;

      private:
        Application() {}
        ~Application() {};

      public:
        Application(Application& application) = delete;
        void operator=(const Application&) = delete;

        static Application& GetInstance();
        static bool GLContextActive();

        // core event functions
        void Init(int argc, char** argv);
        void Start(void);
        void MainEventUpdate(void);
        void PostEventUpdate(void);
        void Clear(void);
    };

}
