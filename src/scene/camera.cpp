#include "pch.h"

#include "core/input.h"
#include "core/clock.h"
#include "core/window.h"
#include "scene/camera.h"

using namespace core;

namespace scene {

    void Camera::Spin(int delta_x, int delta_y) {
        if (delta_x == 0 && delta_y == 0) {
            return;
        }

        euler_y += delta_x * sensitivity;
        euler_x += delta_y * sensitivity;
        euler_x = glm::clamp(euler_x, -88.0f, 88.0f);  // clamp vertical rotation

        forward = glm::normalize(
            glm::vec3(
                cos(glm::radians(euler_y)) * cos(glm::radians(euler_x)),
                sin(glm::radians(euler_x)),
                sin(glm::radians(euler_y)) * cos(glm::radians(euler_x))
            )
        );

        right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, forward));
    }

    void Camera::Zoom(float zoom) {
        fov += zoom * zoom_speed;
        fov = glm::clamp(fov, 1.0f, 90.0f);
    }

    void Camera::Move(Direction direction, float deltatime, bool snap) {
        float elevation = position.y;

        switch (direction) {
            case Direction::F: position += forward * (move_speed * deltatime); break;
            case Direction::B: position -= forward * (move_speed * deltatime); break;
            case Direction::L: position -= right * (move_speed * deltatime); break;
            case Direction::R: position += right * (move_speed * deltatime); break;
            case Direction::U: position.y += move_speed * deltatime; break;
            case Direction::D: position.y -= move_speed * deltatime; break;
        }

        if (snap) {
            position.y = elevation;  // snap to the plane
        }
    }

    Camera::Camera(glm::vec3 position, glm::vec3 forward, float euler_x, float euler_y)
        : position{position}, forward{forward}, euler_x(euler_x), euler_y(euler_y),
          fov(90.0f), near_clip(0.1f), far_clip(100.0f),
          move_speed(1.8f), zoom_speed(1.0f), sensitivity(0.2f) {

        right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, forward));
    }

    glm::mat4 Camera::GetViewMatrix() const {
        return glm::lookAt(position, position + forward, up);
    }

    glm::mat4 Camera::GetProjectionMatrix() const {
        return glm::perspective(glm::radians(fov), Window::aspect_ratio, near_clip, far_clip);
    }

    void Camera::Update(bool snap) {
        Spin(Input::ReadMouseAxis(Axis::X), Input::ReadMouseAxis(Axis::Y));
        Zoom(Input::ReadMouseZoom());

        float deltatime = Clock::delta_time;

        if (Input::IsKeyPressed('w') || Input::IsKeyPressed(GLUT_KEY_UP)) {
            Move(Direction::F, deltatime, snap);
        }

        if (Input::IsKeyPressed('s') || Input::IsKeyPressed(GLUT_KEY_DOWN)) {
            Move(Direction::B, deltatime, snap);
        }

        if (Input::IsKeyPressed('a') || Input::IsKeyPressed(GLUT_KEY_LEFT)) {
            Move(Direction::L, deltatime, snap);
        }

        if (Input::IsKeyPressed('d') || Input::IsKeyPressed(GLUT_KEY_RIGHT)) {
            Move(Direction::R, deltatime, snap);
        }

        if (Input::IsKeyPressed(VK_SPACE)) {
            Move(Direction::U, deltatime, snap);
        }

        if (Input::IsKeyPressed('z')) {
            Move(Direction::D, deltatime, snap);
        }
    }
}
