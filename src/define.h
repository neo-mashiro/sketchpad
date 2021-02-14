#pragma once

#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

constexpr auto PI = 3.14159265358979323846;

typedef GLfloat vertex2[2];
typedef GLfloat vertex3[3];
typedef GLfloat vertex4[4];

extern std::string window_title;
extern unsigned int display_mode;
extern int window_width;
extern int window_height;

void Init(void);
void Display(void);
void Reshape(int width, int height);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Idle(void);
void Motion(int x, int y);
void PassiveMotion(int x, int y);