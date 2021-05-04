#pragma once

#include <cmath>
#include <iostream>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "canvas.h"

enum class Direction { W, A, S, D };

class Camera {
  private:
    glm::vec3 position;  // camera position in world space
    glm::vec3 forward;   // forward direction in world space
    glm::vec3 right;     // right direction in world space
    glm::vec3 up;        // up direction in world space
    float euler_x;       // euler angles around the x-axis
    float euler_y;       // euler angles around the y-axis

    void Spin(int delta_x, int delta_y);
    void Zoom(int zoom);
    void Move(Direction direction, float deltatime, bool snap);

  public:
    float fov;           // vertical field of view (fovy)
    float near_clip;     // near clipping distance
    float far_clip;      // far clipping distance
    float move_speed;    // keypress translates the camera
    float zoom_speed;    // scrollwheel zooms in/out the FoV
    float sensitivity;   // mouse movement rotates the camera

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f),
        float euler_x = 0.0f, float euler_y = -90.0f);

    glm::mat4 GetViewMatrix(void) const;
    glm::mat4 GetProjectionMatrix(float aspect_ratio = 1.0f) const;

    void Update(Canvas::MouseState mouse, int zoom, Canvas::KeyState keystate, float deltatime, bool snap = true);
};
