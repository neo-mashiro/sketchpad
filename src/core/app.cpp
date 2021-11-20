#include "pch.h"

#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

#include "core/app.h"
#include "core/clock.h"
#include "core/event.h"
#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/filesystem.h"

using namespace scene;

namespace core {

    // singleton instance accessor
    Application& Application::GetInstance() {
        // since C++11, this will be thread-safe, there's no need for manual locking
        static Application instance;
        return instance;
    }

    // check if the OpenGL context is active
    bool Application::GLContextActive() { return GetInstance().gl_context_active; }

    #pragma warning(push)
    #pragma warning(disable : 4100)

    // OpenGL debug message callback
    static void GLAPIENTRY OnErrorDetect(GLenum source, GLenum type, GLuint id,
        GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {

        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;  // silently ignore notifications
        }

        std::string err_code;
        GLenum err;

        while ((err = glGetError()) != GL_NO_ERROR) {
            switch (err) {
                case GL_INVALID_ENUM:                  err_code = "invalid enumeration";      break;
                case GL_INVALID_VALUE:                 err_code = "invalid parameter value";  break;
                case GL_INVALID_OPERATION:             err_code = "invalid operation";        break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: err_code = "invalid framebuffer";      break;
                case GL_OUT_OF_MEMORY:                 err_code = "cannot allocate memory";   break;
                case GL_CONTEXT_LOST:                  err_code = "OpenGL context lost";      break;
                case GL_STACK_OVERFLOW:                err_code = "stack overflow";           break;
                case GL_STACK_UNDERFLOW:               err_code = "stack underflow";          break;
                default:                               err_code = "unknown error";            break;
            }

            CORE_ERROR("Internal error detected! {0:x}: {1}", err, err_code);
        }

        std::string err_source, err_type, err_level;

        switch (source) {
            case GL_DEBUG_SOURCE_API:             err_source = "OpenGL API calls";  break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   err_source = "Windows API calls"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: err_source = "shader compiler";   break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     err_source = "third party";       break;
            case GL_DEBUG_SOURCE_APPLICATION:     err_source = "main application";  break;
            case GL_DEBUG_SOURCE_OTHER:           err_source = "other";             break;
            default:                              err_source = "???";               break;
        }

        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               err_type = "error";               break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: err_type = "deprecated behavior"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  err_type = "undefined behavior";  break;
            case GL_DEBUG_TYPE_PORTABILITY:         err_type = "portability";         break;
            case GL_DEBUG_TYPE_PERFORMANCE:         err_type = "performance";         break;
            case GL_DEBUG_TYPE_OTHER:               err_type = "other";               break;
            default:                                err_type = "???";                 break;
        }

        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:         err_level = "high";         break;
            case GL_DEBUG_SEVERITY_MEDIUM:       err_level = "medium";       break;
            case GL_DEBUG_SEVERITY_LOW:          err_level = "low";          break;
            default:                             err_level = "???";          break;
        }

        CORE_ERROR("Debug message callback has been triggered!");
        CORE_ERROR("Error source:   {0}", err_source);
        CORE_ERROR("Error type:     {0}", err_type);
        CORE_ERROR("Error severity: {0}", err_level);
        CORE_TRACE("Error message:  {0}", message);

        // if the breakpoint is triggered, check the "Stack Frame" dropdown list above
        // to find out exactly which source file and line number has caused the error.
        // please also read the detailede error code and message in the console, FAQ:
        // https://www.khronos.org/opengl/wiki/OpenGL_Error

        if constexpr (debug_mode) {
            if (err_level == "high") {
                __debugbreak();  // this is the MSVC intrinsic
            }
        }
    }

    #pragma warning(pop)

    // core event functions
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
            CORE_ASERT(glfwInit() == GLFW_TRUE, "Fatal: Unable to initialize GLFW ...");
        }
        
        CORE_INFO("Creating application main window ...");
        Window::Init();

        // note that when using freeglut, glad can only load the compatibility profile
        // if we only want to work with the core profile, GLFW is the option to choose

        CORE_INFO("Loading OpenGL core profile specs ...");
        if constexpr (_freeglut) {
            CORE_ASERT(gladLoadGL(), "Failed to initialize GLAD with the internal loader...");
        }
        else {
            CORE_ASERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD...");
        }

        CORE_INFO("Initializing Dear ImGui backends ...");
        ui::Init();

        // note that freeglut may not be able to create a debug context on some drivers, so
        // we could lose the ability to register the debug message callback. This is not an
        // error, but the limitation of freeglut. On both my Intel and ATI card, setting the
        // context flag `glutInitContextFlags(GLUT_DEBUG)` has no effect.
        // using GLFW on the other hand, is guaranteed to provide us a valid debug context.

        CORE_INFO("Starting application debug session ...");
        int context_flag;
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flag);

        if constexpr (debug_mode) {
            if (context_flag & GL_CONTEXT_FLAG_DEBUG_BIT) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(OnErrorDetect, nullptr);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
            }
            else {
                if constexpr (_freeglut) {
                    CORE_WARN("Unable to register the debug message callback ...");
                    CORE_WARN("Debug context may not be available in freeglut ...");
                }
                else {
                    CORE_ERROR("Unable to register the debug message callback ...");
                    CORE_ERROR("Have you hinted GLFW to create a debug context ?");
                }
            }
        }

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
            Window::OnLayerSwitch();
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

        Input::Clear();
        Clock::Reset();
        Window::Clear();
    }
}
