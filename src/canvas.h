#pragma once

#include <iostream>
#include <string>
#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

class Canvas {
  public:
    struct Window {
        int id;  // unique window identifier
        std::string title;
        unsigned int width, height;
        float aspect_ratio;
        int zoom, pos_x, pos_y;
        unsigned int display_mode;
    };

    struct FrameCounter {
        float last_frame, this_frame, delta_time;
    };

    struct MouseState {
        unsigned int pos_x, pos_y;
        int delta_x, delta_y;
    };

    struct KeyState {
        bool u, d, l, r;
    };

    struct Window window;
    struct FrameCounter frame_counter;
    struct MouseState mouse;
    struct KeyState keystate;

    Canvas(const std::string& title, unsigned int width = 1280, unsigned int height = 720);
    ~Canvas();

    void Update();

    // default callbacks
    virtual void Idle(void);
    virtual void Entry(int state);
    virtual void Keyboard(unsigned char key, int x, int y);
    virtual void Reshape(int width, int height);
    virtual void PassiveMotion(int x, int y);
    virtual void Mouse(int button, int state, int x, int y);
    virtual void Special(int key, int x, int y);
    virtual void SpecialUp(int key, int x, int y);
};
