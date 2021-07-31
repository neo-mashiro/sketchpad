#pragma once

#include "scene/transform.h"

namespace scene {

    class Camera : public Transform {
      public:
        float fov;           // field of view (vertical)
        float near_clip;     // near clipping distance
        float far_clip;      // far clipping distance
        float move_speed;
        float rotate_speed;

        Camera();
        ~Camera() {}

        glm::mat4 GetViewMatrix(void) const;
        glm::mat4 GetProjectionMatrix(void) const;

        void Update(void);
    };
}
