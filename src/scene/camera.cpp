#include "pch.h"

#define GLM_ENABLE_EXPERIMENTAL

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/perpendicular.hpp>
#pragma warning(pop)

#include "core/input.h"
#include "core/clock.h"
#include "core/window.h"
#include "scene/camera.h"

using namespace core;

namespace scene {

    Camera::Camera()
        : fov(90.0f), near_clip(0.1f), far_clip(100.0f), move_speed(3.0f), rotate_speed(0.3f) {}

    glm::mat4 Camera::GetViewMatrix() const {
        return glm::lookAt(position, position + forward, up);
    }

    glm::mat4 Camera::GetProjectionMatrix() const {
        return glm::perspective(glm::radians(fov), Window::aspect_ratio, near_clip, far_clip);
    }

    void Camera::Update() {
        // rotation (based on our euler angles convention we need to invert the axis)
        float euler_y = rotation.y - Input::GetMouseAxis(Axis::Horizontal) * rotate_speed;
        float euler_x = rotation.x - Input::GetMouseAxis(Axis::Vertical) * rotate_speed;
        euler_x = glm::clamp(euler_x, -88.0f, 88.0f);  // clamp vertical rotation

        Rotate(glm::vec3(euler_x, euler_y, 0.0f));

        // camera zoom
        fov += Input::GetMouseZoom();
        fov = glm::clamp(fov, 1.0f, 90.0f);

        // translation
        float deltatime = Clock::delta_time;

        if (Input::IsKeyPressed('w')) {
            Translate(forward * (move_speed * deltatime));
        }

        if (Input::IsKeyPressed('s')) {
            Translate(-forward * (move_speed * deltatime));
        }

        if (Input::IsKeyPressed('a')) {
            Translate(-right * (move_speed * deltatime));
        }

        if (Input::IsKeyPressed('d')) {
            Translate(right * (move_speed * deltatime));
        }

        if (Input::IsKeyPressed('z')) {
            Translate(-up * (move_speed * deltatime));
        }

        if (Input::IsKeyPressed(VK_SPACE)) {
            Translate(up * (move_speed * deltatime));
        }
    }
}
