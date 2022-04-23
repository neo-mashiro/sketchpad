#include "pch.h"

#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

#include "core/app.h"
#include "core/clock.h"
#include "core/debug.h"
#include "core/event.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "scene/renderer.h"
#include "scene/scene.h"
#include "scene/ui.h"
#include "utils/path.h"

using namespace scene;

namespace core {

    Application& Application::GetInstance() {
        // since C++11, this will be thread-safe, there's no need for manual locking
        static Application instance;
        return instance;
    }

    bool Application::GLContextActive() { return GetInstance().gl_context_active; }

    void Application::Init(int argc, char** argv) {
        this->gl_context_active = false;
        std::cout << "Initializing console logger ...\n" << std::endl;
        Log::Init();

        CORE_INFO("Searching sources and assets path tree ...");
        utils::paths::SearchPaths();

        CORE_INFO("Initializing window utility library ...");
        if constexpr (_freeglut) {
            glutInit(&argc, argv);
        }
        else {
            int ret = glfwInit();
            CORE_ASERT(ret == GLFW_TRUE, "Fatal: Unable to initialize GLFW ...");
        }
        
        CORE_INFO("Creating application main window ...");
        Window::Init();

        // note that when using freeglut, glad can only load the compatibility profile
        // if we only want to work with the core profile, GLFW is the option to choose

        CORE_INFO("Loading OpenGL core profile specs ...");
        if constexpr (_freeglut) {
            int ret = gladLoadGL();
            CORE_ASERT(ret, "Failed to initialize GLAD with the internal loader...");
        }
        else {
            int ret = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
            CORE_ASERT(ret, "Failed to initialize GLAD...");
        }

        CORE_INFO("Initializing Dear ImGui backends ...");
        ui::Init();

        CORE_INFO("Starting application debug session ...");
        Debug::RegisterDebugMessageCallback();

        CORE_INFO("Registering window event callbacks ...");
        Event::RegisterCallbacks();

        CORE_INFO("Retrieving hardware specifications ...");
        gl_vendor    = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        gl_renderer  = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        gl_version   = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        glsl_version = std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

        // texture size limit, max number of texture units and image units
        glGetIntegerv(GL_MAX_TEXTURE_SIZE,          &gl_texsize);
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE,       &gl_texsize_3d);
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &gl_texsize_cubemap);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,   &gl_max_texture_units);
        glGetIntegerv(GL_MAX_IMAGE_UNITS,           &gl_max_image_units);

        // max number of atomic counters in each shader stage
        glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &gl_maxv_atcs);
        glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &gl_maxf_atcs);
        glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, &gl_maxc_atcs);

        // max number of uniform blocks in each shader stage
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS,   &gl_maxv_ubos);
        glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &gl_maxg_ubos);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &gl_maxf_ubos);
        glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS,  &gl_maxc_ubos);

        // max number of shader storage blocks in the fragment shader and compute shader
        glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &gl_maxf_ssbos);
        glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,  &gl_maxc_ssbos);

        // max number of work groups in the compute shader
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cs_nx);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &cs_ny);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &cs_nz);

        // compute shader work groups size limit
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &cs_sx);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &cs_sy);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &cs_sz);

        // max number of threads in the compute shader
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &cs_max_invocations);

        // max number of drawable color buffers in a user-defined framebuffer
        GLint max_color_attachments, max_draw_buffers;
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
        gl_max_color_buffs = std::min(max_color_attachments, max_draw_buffers);

        // opengl context is now active, ready to startup
        this->gl_context_active = true;
        this->app_pause = false;
        this->app_shutdown = false;

        std::cout << std::endl;
    }

    void Application::Start() {
        Clock::Reset();
        Input::Clear();
        Input::HideCursor();
        Renderer::Attach("Welcome Screen");
    }

    void Application::MainEventUpdate() {
        if constexpr (_freeglut) {
            glutMainLoopEvent();
        }
        else {
            Renderer::DrawScene();
        }
    }

    void Application::PostEventUpdate() {
        // check if the user has requested to exit
        if (Input::GetKeyDown(VK_ESCAPE)) {
            app_shutdown = Window::OnExitRequest();
            Input::SetKeyDown(VK_ESCAPE, false);  // release the esc key
        }

        // check if the imgui layer has been toggled
        else if (Input::GetKeyDown(VK_RETURN)) {
            if (Renderer::GetScene()->title != "Welcome Screen") {
                Window::OnLayerSwitch();
            }
            Input::SetKeyDown(VK_RETURN, false);  // release the enter key
        }

        if (app_shutdown) {
            GetInstance().Clear();
            exit(EXIT_SUCCESS);
        }

        Clock::Update();
        Renderer::DrawImGui();
    }

    void Application::Clear() {
        CORE_TRACE("Application running time: {0:.2f} seconds", Clock::time);
        CORE_TRACE("Shutting down application ...");

        ui::Clear();

        Renderer::Detach();
        Renderer::Reset();

        Input::Clear();
        Clock::Reset();
        Window::Clear();
        Log::Shutdown();
    }
}
