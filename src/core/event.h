#pragma once

struct GLFWwindow;  // forward declaration

namespace core {

    class Event {
      public:
        static void RegisterCallbacks();

      private:
        static void GlutIdle(void);
        static void GlutDisplay(void);
        static void GlutEntry(int state);
        static void GlutKeyDown(unsigned char key, int x, int y);
        static void GlutKeyUp(unsigned char key, int x, int y);
        static void GlutReshape(int width, int height);
        static void GlutMotion(int x, int y);
        static void GlutPassiveMotion(int x, int y);
        static void GlutMouse(int button, int state, int x, int y);
        static void GlutSpecial(int key, int x, int y);
        static void GlutSpecialUp(int key, int x, int y);

      private:
        static void GlfwError(int error, const char* description);
        static void GlfwCursorEnter(GLFWwindow* window, int entered);
        static void GlfwCursorPos(GLFWwindow* window, double xpos, double ypos);
        static void GlfwMouseButton(GLFWwindow* window, int button, int action, int mods);
        static void GlfwScroll(GLFWwindow* window, double xoffset, double yoffset);
        static void GlfwKey(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void GlfwWindowSize(GLFWwindow* window, int width, int height);
        static void GlfwFramebufferSize(GLFWwindow* window, int width, int height);
        static void GlfwWindowFocus(GLFWwindow* window, int focused);
    };

}
