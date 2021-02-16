#pragma once

#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

constexpr auto PI = 3.14159265358979323846;

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
};

extern Window window;

extern GLuint VAO;  // vertex array object
extern GLuint VBO;  // vertex buffer object
extern GLuint PO;   // program object

void SetupWindow(void);
void Init(void);
void Display(void);
void Reshape(int width, int height);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Idle(void);
void Motion(int x, int y);
void PassiveMotion(int x, int y);
