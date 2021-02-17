#pragma once

#include "define.h"

GLuint CreateProgram(const std::string& shader_path);

void SetupDefaultWindow(Window* window);
void DefaultReshapeCallback(int width, int height);
void DefaultKeyboardCallback(unsigned char key, int x, int y);