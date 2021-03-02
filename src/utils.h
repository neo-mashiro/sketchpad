#pragma once

#include "define.h"

constexpr auto PI = 3.14159265358979323846f;
constexpr auto DEG2RAD = PI / 180.0f;

GLuint CreateShader(const std::string& shader_path);

void SetupDefaultWindow(Window& window);
void DefaultReshapeCallback(Window& window, int width, int height);
void DefaultKeyboardCallback(unsigned char key, int x, int y);
void DefaultEntryCallback(int state);