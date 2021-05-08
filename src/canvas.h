#pragma once

#include <iostream>
#include <string>
#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

struct Window {
    int id;
    const char* title;
    GLuint width, height;
    GLuint full_width, full_height;
    float aspect_ratio;
    int zoom, pos_x, pos_y;
    GLuint display_mode;
};

struct FrameCounter {
    float last_frame, this_frame, delta_time;
};

struct MouseState {
    GLuint pos_x, pos_y;
    int delta_x, delta_y;
};

struct KeyState {
    bool u, d, l, r;
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

    static void CheckOpenGLContext(const std::string& call);

    static void Update();

    // default event callbacks
    static void Idle(void);
    static void Entry(int state);
    static void Keyboard(unsigned char key, int x, int y);
    static void Reshape(int width, int height);
    static void PassiveMotion(int x, int y);
    static void Mouse(int button, int state, int x, int y);
    static void Special(int key, int x, int y);
    static void SpecialUp(int key, int x, int y);
};