#include "camera.h"
#include "log.h"

namespace Sketchpad {

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

    void Camera::Zoom(int zoom) {
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

            case Direction::U:
                position.y += move_speed * deltatime;
                snap = false;
                break;

            case Direction::D:
                position.y -= move_speed * deltatime;
                position.y = std::max(position.y, 0.0f);
                snap = false;
                break;
        }

        if (snap) {
            position.y = elevation;  // snap to the plane
        }
    }

    Camera::Camera(glm::vec3 position, glm::vec3 forward, float euler_x, float euler_y)
        : position{position}, forward{forward}, euler_x(euler_x), euler_y(euler_y), fov(90.0f),
          near_clip(0.1f), far_clip(100.0f), move_speed(1.8f), zoom_speed(1.0f), sensitivity(0.2f) {

        right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, forward));
    }

    glm::mat4 Camera::GetViewMatrix() const {
        return glm::lookAt(position, position + forward, up);
    }

    glm::mat4 Camera::GetProjectionMatrix(float aspect_ratio) const {
        return glm::perspective(glm::radians(fov), aspect_ratio, near_clip, far_clip);
    }

    void Camera::Update(MouseState& mouse, Window& window, KeyState& keystate, float deltatime, bool snap) {
        Spin(mouse.delta_x, mouse.delta_y);
        mouse.delta_x = mouse.delta_y = 0;  // recover mouse offset

        Zoom(window.zoom);
        window.zoom = 0;  // recover zoom to 0

        if (keystate.f) Move(Direction::F, deltatime, snap);
        if (keystate.b) Move(Direction::B, deltatime, snap);
        if (keystate.l) Move(Direction::L, deltatime, snap);
        if (keystate.r) Move(Direction::R, deltatime, snap);
        if (keystate.u) Move(Direction::U, deltatime, snap);
        if (keystate.d) Move(Direction::D, deltatime, snap);
    }
}
