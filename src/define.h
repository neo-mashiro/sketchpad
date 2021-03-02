#pragma once

#include <cstdio>
#include <cmath>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/perpendicular.hpp>

typedef GLfloat vertex2[2];
typedef GLfloat vertex3[3];
typedef GLfloat vertex4[4];

struct Window {
    int id{ -1 };
    const char* title{"Canvas"};
    unsigned int display_mode{ GLUT_SINGLE | GLUT_RGB };
    unsigned int pos_x{ 0 };
    unsigned int pos_y{ 0 };
    unsigned int width{ 800 };
    unsigned int height{ 800 };
    float aspect_ratio{ 1.0f };
};

struct Camera {
    glm::vec3 position{ glm::vec3(0.0f, 0.0f, 3.0f) };
    glm::vec3 forward{ glm::vec3(0.0f, 0.0f, -1.0f) };
    glm::vec3 right{ glm::vec3(1.0f, 0.0f, 0.0f) };
    glm::vec3 up{ glm::vec3(0.0f, 1.0f, 0.0f) };
    float euler_x{ 0.0f };
    float euler_y{ -90.0f };
    float fov{ 90.0f };
    float speed{ 2.5f };
};

struct FrameCounter {
    float delta_time{ 0.0f };
    float last_frame{ 0.0f };
    float this_frame{ 0.0f };
};

struct MouseState {
    float sensitivity{ 0.05f };
    float zoom_speed{ 2.0f };
    int last_x{ 0 };
    int last_y{ 0 };
};

struct KeyState {
    bool up{ false };
    bool down{ false };
    bool left{ false };
    bool right{ false };
};

struct Textures {
    GLuint base{ 0 };
    GLuint normal{ 0 };
    GLuint height{ 0 };
    GLuint occlusion{ 0 };
    GLuint roughness{ 0 };
};

void SetupWindow(void);
void Init(void);
void Display(void);
void Reshape(int width, int height);
void Keyboard(unsigned char key, int x, int y);
void Special(int key, int x, int y);
void SpecialUp(int key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Idle(void);
void Entry(int state);
void Motion(int x, int y);
void PassiveMotion(int x, int y);
void Cleanup(void);