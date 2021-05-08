#include "camera.h"

void Camera::Spin(int delta_x, int delta_y) {
    if (delta_x == 0 && delta_y == 0) {
        return;
    }

    euler_y += delta_x * sensitivity;
    euler_x += delta_y * sensitivity;
    euler_x = glm::clamp(euler_x, -90.0f, 90.0f);  // clamp vertical rotation

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
    switch (direction) {
    	case Direction::W: position += forward * (move_speed * deltatime); break;
        case Direction::S: position -= forward * (move_speed * deltatime); break;
    	case Direction::A: position -= right * (move_speed * deltatime); break;
        case Direction::D: position += right * (move_speed * deltatime); break;

    	default:
            std::cout << "[WARNING] Invalid direction" << std::endl; break;
	}

    if (snap) {
        position.y = 0.0f;  // snap to the ground
    }
}

Camera::Camera(glm::vec3 position, glm::vec3 forward, float euler_x, float euler_y)
    : position{position}, forward{forward}, euler_x(euler_x), euler_y(euler_y), fov(90.0f),
      near_clip(0.1f), far_clip(100.0f), move_speed(2.5f), zoom_speed(2.0f), sensitivity(0.05f) {

    right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, forward));
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(position, position + forward, up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspect_ratio) const {
    return glm::perspective(glm::radians(fov), aspect_ratio, near_clip, far_clip);
}

void Camera::Update(MouseState mouse, int zoom, KeyState keystate, float deltatime, bool snap) {
    Spin(mouse.delta_x, mouse.delta_y);
    Zoom(zoom);
    if (keystate.u) Move(Direction::W, deltatime, snap);
    if (keystate.d) Move(Direction::S, deltatime, snap);
    if (keystate.l) Move(Direction::A, deltatime, snap);
    if (keystate.r) Move(Direction::D, deltatime, snap);
}
