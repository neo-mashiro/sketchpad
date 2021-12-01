#pragma once

#include <glad/glad.h>

namespace core {

    /* modern OpenGL applications use the debug message callback for catching errors,
       this callback is a convenient way for debugging, very much like the GLFW error
       callback. In order to register this callback successfully a valid OpenGL debug
       context must be present first. However, such context may not be available when
       using freeglut on some drivers, so the callback is not always triggered (GLFW
       seems to be working on all drivers all the time). In case it's not 100 percent
       reliable, we've also provided a `CheckGLError()` function that allows users to
       check errors manually when debugging the scene code.

       the `CheckGLError()` function can be used in conjunction with the debug message
       callback to ensure 100% bug-free. You don't need to call it after every line of
       code, it's intended to be placed uniformly across the scene code, each call may
       have a unique "checkpoint" number that will be logged to the console when error
       occurs. This way, we can quicky find out which call is closest to the error.

       there may be cases when we want to create an error on purpose, or know about an
       error that happens all the time but is irrelevant to our own code (e.g. caused
       by a third-party library). In this case, we can discard an error manually using
       `CheckGLError(-1)`. By passing a checkpoint of -1, the function will ignore the
       error and suppress the console message silently.

       # tips on debugging

       the debug message callback only ensures that the calls are correct and our code
       compiles and runs, but there's no guarantee on the correctness of visual output.
       it's not rare that we end up with a black screen or any visual artifact without
       a single OpenGL error, or even worse, the observed behavior isn't consistent on
       other platforms, or some function just hangs forever. There can be many reasons
       for this, some of the most common ones are:

       > incorrect data and image internal format
       > corrupted buffer data or shader input
       > vertices out of clip space, colors clamped to [0,1]
       > memory barrier or synchronization chaos
       > context reset or known hardware issues
       > wrong rasterizer state (fails depth testing, faces being culled, etc) **
       > ......

       when one or more of these occurs, debugging can be very hard and time-consuming.
       there's no way to avoid such bugs, but we do have some best practices to follow.
       usually it's wise to have at least one reference environment, for example, we
       might want to test our app on a different driver, or check to see if our light
       shader yields similar visual output in Unity and Unreal. As graphics developers,
       we must bear in mind that the built-in tools are only a start. As our project
       becomes more and more complex, ultimately we would need some help from external
       debugging software such as "RenderDoc", so we'd better learn to use them asap...
    */

    class Debug {
      public:
        static void CheckGLContext();
        static void CheckGLError(int checkpoint);
        static void CatchGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
            const GLchar* message, const void* userParam);
    };

    class NotImplementedError : public std::logic_error {
      public:
        NotImplementedError(const char* message) : std::logic_error(message) {};
    };

}
