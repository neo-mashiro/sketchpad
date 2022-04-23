#include "pch.h"

#include "core/debug.h"
#include "core/log.h"

namespace core {

    #pragma warning(push)
    #pragma warning(disable : 4100)

    void Debug::DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, const GLchar* message, const void* userParam) {

        CheckGLContext();  // first check if hardware state has been reset (e.g. AMD timeout)

        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;  // silently ignore notifications
        }

        // ignore notification and driver bugs that are misreported as "errors" by some drivers
        switch (id) {
            case 131185: return;  // NVIDIA: ImGui texture buffers use GL_STREAM_DRAW draw hint
            case 131204: return;  // NVIDIA: texture image unit unbound to 0
            default:
                break;
        }

        std::string err_text;
        GLenum err_code;

        while ((err_code = glGetError()) != GL_NO_ERROR) {
            switch (err_code) {
                case GL_INVALID_ENUM:                  err_text = "invalid enumeration";      break;
                case GL_INVALID_VALUE:                 err_text = "invalid parameter value";  break;
                case GL_INVALID_OPERATION:             err_text = "invalid operation";        break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: err_text = "invalid framebuffer";      break;
                case GL_OUT_OF_MEMORY:                 err_text = "cannot allocate memory";   break;
                case GL_CONTEXT_LOST:                  err_text = "OpenGL context lost";      break;
                case GL_STACK_OVERFLOW:                err_text = "stack overflow";           break;
                case GL_STACK_UNDERFLOW:               err_text = "stack underflow";          break;
                default:                               err_text = "unknown error";            break;
            }
            CORE_ERROR("Internal error detected! {0:x}: {1}", err_code, err_text);  // error code in hex
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

        CORE_ERROR("OpenGL debug message callback has been triggered!");
        CORE_ERROR("Message id:     {0} (implementation defined)", id);
        CORE_ERROR("Error source:   {0}", err_source);
        CORE_ERROR("Error type:     {0}", err_type);
        CORE_ERROR("Error severity: {0}", err_level);
        CORE_ERROR("Error message:  {0}", message);

        // if the breakpoint is triggered, check the "Stack Frame" dropdown list above
        // to find out exactly which source file and line number has caused the error.
        // see also: https://www.khronos.org/opengl/wiki/OpenGL_Error

        if constexpr (debug_mode) {
            if (err_level == "high") {
                SP_DBG_BREAK();  // this is the MSVC intrinsic
            }
        }
    }

    #pragma warning(pop)

    void Debug::CheckGLContext() {
        GLenum status = glGetGraphicsResetStatus();
        if (status == GL_NO_ERROR) {
            return;
        }

        if (status == GL_GUILTY_CONTEXT_RESET) {
            CORE_ERROR("GL context lost due to a hang caused by the client!");
            CORE_ERROR("Do you have an infinite loop in GLSL that locked up the machine?");
        }
        else if (status == GL_INNOCENT_CONTEXT_RESET) {
            CORE_ERROR("GL context lost due to a reset caused by some other process!");
            CORE_ERROR("Please restart the application and reinitialize the context.");
        }
        else if (status == GL_UNKNOWN_CONTEXT_RESET) {
            // if the hang persists, our code has fatal errors, use "RenderDoc" to find out
            CORE_ERROR("A graphics reset has been detected whose cause is unknown!");
            CORE_ERROR("Please restart the application to see if the hang persists.");
        }

        CORE_ERROR("Fatal error detected, aborting application ...");
        SP_DBG_BREAK();
    }

    void Debug::CheckGLError(int checkpoint) {
        // to ignore an error and suppress the message, pass a checkpoint of -1
        GLenum err_code;
        while ((err_code = glGetError()) != GL_NO_ERROR) {
            if (err_code != -1) {
                CORE_ERROR("OpenGL error detected at checkpoint {0}: {1}", checkpoint, err_code);
            }
        }
    }

    void Debug::RegisterDebugMessageCallback() {
        int context_flag;
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flag);

        // note that freeglut may not be able to create a debug context on some drivers, so
        // we could lose the ability to register the debug message callback. This is not an
        // error, but the limitation of freeglut on certain drivers. In this case, setting
        // the context flag `glutInitContextFlags(GLUT_DEBUG)` in window creation will have
        // no effect. GLFW3 on the other hand, can always provide a valid debug context.

        if constexpr (debug_mode) {
            if (context_flag & GL_CONTEXT_FLAG_DEBUG_BIT) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback((GLDEBUGPROC)DebugMessageCallback, nullptr);
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
    }

}
