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

typedef GLfloat vertex2[2];
typedef GLfloat vertex3[3];
typedef GLfloat vertex4[4];

struct Window {
    int id{ -1 };
    const char* title{"Canvas"};
    unsigned int display_mode{ GLUT_SINGLE | GLUT_RGB };
    unsigned int pos_x{ 0 };
    unsigned int pos_y{ 0 };
    unsigned int width{ 512 };
    unsigned int height{ 512 };
    float aspect_ratio{ 1.0f };
};

// assume only one window for simplicity
extern Window window;

void SetupWindow(void);
void Init(void);
void Display(void);
void Reshape(int width, int height);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Idle(void);
void Motion(int x, int y);
void PassiveMotion(int x, int y);
void Cleanup(void);