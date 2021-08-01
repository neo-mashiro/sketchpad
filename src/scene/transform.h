#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace scene {

    namespace world {
        // OpenGL adopts a right-handed coordinate system
        constexpr glm::vec3 origin  = glm::vec3(0.0f);
        constexpr glm::vec3 zero    = glm::vec3(0.0f);
        constexpr glm::vec3 unit    = glm::vec3(1.0f);
        constexpr glm::mat4 eye     = glm::mat4(1.0f);
        constexpr glm::vec3 up      = { 0.0f, 1.0f, 0.0f };
        constexpr glm::vec3 forward = { 0.0f, 0.0f, 1.0f };
        constexpr glm::vec3 right   = { -1.0f, 0.0f, 0.0f };
    }

    class Transform {
      private:
        void RecalculateBasis(void);

      protected:
        // make the ctor protected to prevent the class from being instantiated
        Transform();
        ~Transform() {}

      public:
        glm::vec3 position;
        glm::vec3 rotation;   // euler angles in degrees (pitch, yawn, roll)
        glm::vec3 scale;

        glm::mat4 transform;  // 4x4 homogeneous matrix stored in column-major order

        glm::vec3 up;         // local up direction
        glm::vec3 forward;    // local forward direction
        glm::vec3 right;      // local right direction

        // transformation functions
        void Translate(const glm::vec3& vec);
        void Rotate(float radians, const glm::vec3& axis);
        void Rotate(const glm::vec3& degrees);
        void Scale(float factor);
    };

}
