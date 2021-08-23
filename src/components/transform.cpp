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

    // we will only allow uniform scaling so as to keep the transform matrix orthogonal, this way,
    // it's easier to transform a normal vector (just multiply it by the transform), there's no
    // need to calculate the transpose of its inverse. Another benefit is that, uniform scaling
    // and rotation always commute, so we are able to apply them in an arbitrary order.
    void Transform::Scale(float factor) {
        transform = glm::scale(transform, glm::vec3(factor));
        this->scale *= factor;
    }

    // we have provided two rotate functions here, but keep in mind that euler angles
    // are evil, whenever possible, you should use the first function. The second one
    // is intended for setting rotations directly (absolute change), such as when you
    // want to set an object's initial orientation. In this demo, it is primarily used
    // by our camera class because camera rotation needs to be clamped on the vertical
    // axis, and euler angles are easier to clamp than matrices or quaternions.

    // using the second function could be dangerous as rotations in 3D usually do not
    // commute. Also, unlike matrices or quaternions, people like to use different
    // conventions for euler angles and they are not unique, for example, (0, 90, 0)
    // and (-180, 90, 180) could be the same rotation if you follow a specific order.

    // for euler angles, our convention is that a positive angle always corresponds to
    // a counter-clockwise rotation about an axis, and the rotations are applied in
    // the order of yawn->pitch->roll (y->x->z). You should also limit the range of
    // angles to avoid ambiguity, which must work with the glm functions used here.
    // since the order of rotation is y->x->z, the middle one is pitch, pitch should
    // be between (-90, 90) degrees, and yawn and roll should be within (-180, 180).

    // rotation around a given axis (incremental change applied on the current rotation)
    void Transform::Rotate(float radians, const glm::vec3& axis) {
        transform = glm::rotate(transform, radians, axis);
        glm::extractEulerAngleYXZ(transform, rotation.y, rotation.x, rotation.z);
        rotation = glm::degrees(rotation);

        RecalculateBasis();
    }

    // rotation given by a vector of euler angles in degrees (absolute change relative to 0 orientation)
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

    // recalculate the basis vectors based on the rotation matrix (this is more robust than
    // using trigonometric functions, which rely on the conventions of euler angles)
    void Transform::RecalculateBasis() {
        static constexpr glm::vec3 world_up      = { 0.0f, 1.0f, 0.0f };
        static constexpr glm::vec4 world_forward = { 0.0f, 0.0f, 1.0f, 0.0f };

        forward = glm::normalize(glm::vec3(transform * world_forward));
        right = glm::normalize(glm::cross(forward, world_up));
        up = glm::normalize(glm::cross(right, forward));
    }
}
