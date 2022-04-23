/*
   this is a simple FPS camera + the feature of smooth recovery, smooth zooming
   and camera orbiting (arcball camera like the one in Blender), note that it's
   tied to the transform component for doing updates, and every scene must have
   exactly one main camera.

   smooth camera shakes using 3D Perlin noise will be added in the next release.
   (copy from my Unity C# project)
*/

#pragma once

#include <glm/glm.hpp>
#include "component/component.h"

namespace component {

    class Transform;  // forward declaration

    enum class View : char {
        Orthgraphic = 1 << 0,
        Perspective = 1 << 1
    };

    class Camera : public Component {
      public:
        float fov;
        float near_clip;
        float far_clip;
        float move_speed;
        float zoom_speed;
        float rotate_speed;
        float orbit_speed;

        glm::vec3 initial_position;
        glm::quat initial_rotation;

        Transform* T;  // transform is partially owned by ECS groups so this pointer is reliable
        View view;

      public:
        Camera(Transform* T, View view = View::Perspective);

        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix() const;

        void Update();
    };

}
