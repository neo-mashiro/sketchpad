#pragma once

#include <glm/glm.hpp>

namespace components {

    class Transform;  // forward declaration

    enum class View {
        Orthographic,
        Perspective,
    };

    class Camera {
      public:
        float fov;
        float near_clip;
        float far_clip;
        float move_speed;
        float rotate_speed;

        View view;
        Transform* T;

        Camera(Transform* t, View view = View::Perspective);
        ~Camera() {}

        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;

        Camera(Camera&& other) = default;
        Camera& operator=(Camera&& other) = default;

        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix() const;

        void Update();
    };
}
