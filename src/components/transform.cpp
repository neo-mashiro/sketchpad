#include "pch.h"

#include "components/transform.h"

namespace components {

    Transform::Transform() :
        position(0.0f), rotation(0.0f), scale(1.0f), transform(1.0f),
        up(0.0f, 1.0f, 0.0f), forward(0.0f, 0.0f, 1.0f), right(-1.0f, 0.0f, 0.0f) { }

    void Transform::Translate(const glm::vec3& vec) {
        // be aware that the amount of translation will be scaled if scale factor is not 1
        // to compute the correct transform matrix, we need to translate by "vec / scale"
        transform = glm::translate(transform, vec / scale.x);
        position += vec;
    }

    void Transform::Scale(float factor) {
        transform = glm::scale(transform, glm::vec3(factor));
        this->scale *= factor;
    }

    void Transform::Rotate(float radians, const glm::vec3& axis) {
        transform = glm::rotate(transform, radians, axis);
        glm::extractEulerAngleYXZ(transform, rotation.y, rotation.x, rotation.z);
        rotation = glm::degrees(rotation);

        RecalculateBasis();
    }

    void Transform::Rotate(const glm::vec3& degrees) {
        rotation = degrees;  // overwrites the current euler angles

        // the 4x4 transform matrix is stored in column-major order as below
        // where translation, rotation and scaling components are T, R and S
        // [ R11 * S, R12 * S, R13 * S, TX ]
        // [ R21 * S, R22 * S, R23 * S, TY ]
        // [ R31 * S, R32 * S, R33 * S, TZ ]
        // [ 0      , 0      , 0      , 1  ]

        // first we need to remove the old rotation components from it so it looks like
        // [ S,  0,  0,  TX ]
        // [ 0,  S,  0,  TY ]
        // [ 0,  0,  S,  TZ ]
        // [ 0,  0,  0,  1  ]

        glm::vec4 translation = transform[3];
        transform = glm::mat4(1.0f) * scale.x;
        transform[3] = translation;

        // now we can apply the new rotation matrix
        glm::vec3 radians = glm::radians(degrees);
        transform = glm::eulerAngleYXZ(radians.y, radians.x, radians.z) * transform;

        RecalculateBasis();
    }

    void Transform::RecalculateBasis() {
        static constexpr glm::vec3 world_up      = { 0.0f, 1.0f, 0.0f };
        static constexpr glm::vec4 world_forward = { 0.0f, 0.0f, 1.0f, 0.0f };

        forward = glm::normalize(glm::vec3(transform * world_forward));
        right = glm::normalize(glm::cross(forward, world_up));
        up = glm::normalize(glm::cross(right, forward));
    }
}
