#pragma once

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/perpendicular.hpp>
#pragma warning(pop)

namespace scene {

    enum class Direction { F, B, L, R, U, D };

    class Camera {
      private:
        float euler_x;       // euler angles around the x-axis
        float euler_y;       // euler angles around the y-axis

        void Spin(int delta_x, int delta_y);
        void Zoom(float zoom);
        void Move(Direction direction, float deltatime, bool snap);

      public:
        float fov;           // vertical field of view (fovy)
        float near_clip;     // near clipping distance
        float far_clip;      // far clipping distance
        float zoom_speed;    // scrollwheel zooms in/out the FoV
        float move_speed;    // keypress translates the camera
        float sensitivity;   // mouse movement rotates the camera

        glm::vec3 position;  // camera position in world space
        glm::vec3 forward;   // forward direction in world space
        glm::vec3 right;     // right direction in world space
        glm::vec3 up;        // up direction in world space

        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f),
            float euler_x = 0.0f, float euler_y = -90.0f);

        glm::mat4 GetViewMatrix(void) const;
        glm::mat4 GetProjectionMatrix() const;

        void Update(bool snap = false);
    };
}
