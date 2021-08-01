#include "pch.h"

#define GLM_ENABLE_EXPERIMENTAL

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#pragma warning(pop)

#include "scene/transform.h"

namespace scene {

    Transform::Transform() :
        position(world::origin), rotation(world::zero), scale(world::unit), transform(world::eye),
        up(world::up), forward(world::forward), right(world::right) {}

    void Transform::Translate(const glm::vec3& vec) {
        // be aware that the amount of translation will be scaled if scale factor is not 1
        // to compute the correct transform matrix, we need to translate by "vec / scale"
        transform = glm::translate(transform, vec / scale.x);
        position += vec;
    }

    void Transform::Scale(float factor) {
        // we only allow uniform scaling so as to keep the transform matrix orthogonal,
        // this way, we can easily transform a normal vector by multiplying it by the
        // transform itself (model matrix), without using the transpose of its inverse.
        // another benefit is that, rotation and uniform scaling always commute, so we
        // are able to apply them in an arbitrary order.
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

        // the 4ž4 transform matrix is stored in column-major order as below
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
        transform = world::eye * scale.x;
        transform[3] = translation;

        // now we can apply the new rotation matrix
        glm::vec3 radians = glm::radians(degrees);
        transform = glm::eulerAngleYXZ(radians.y, radians.x, radians.z) * transform;

        RecalculateBasis();
    }

    void Transform::RecalculateBasis() {
        // since euler angles are evil (different conventions, gimbal lock, order ambiguity...)
        // we will use the rotation matrix to calculate our basis (local direction vectors)
        // rather than using trigonometric functions, which rely on the correctness of euler angles

        forward = glm::normalize(glm::vec3(transform * glm::vec4(world::forward, 0.0f)));
        right = glm::normalize(glm::cross(forward, world::up));
        up = glm::normalize(glm::cross(right, forward));
    }
}
