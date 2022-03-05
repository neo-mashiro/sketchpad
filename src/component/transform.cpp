#include "pch.h"

#include "component/transform.h"
#include "utils/math.h"

namespace component {

    static constexpr glm::vec3 origin        { 0.0f };
    static constexpr glm::mat4 identity      { 1.0f };
    static constexpr glm::quat eye           { 1.0f, 0.0f, 0.0f, 0.0f };  // wxyz
    static constexpr glm::vec3 world_right   { 1.0f, 0.0f, 0.0f };  // +x axis
    static constexpr glm::vec3 world_up      { 0.0f, 1.0f, 0.0f };  // +y axis
    static constexpr glm::vec3 world_forward { 0.0f, 0.0f,-1.0f };  // -z axis

    Transform::Transform() :
        Component(), transform(identity), position(origin), rotation(eye),
        right(world_right), up(world_up), forward(world_forward),
        euler_x(0.0f), euler_y(0.0f), euler_z(0.0f),
        scale_x(1.0f), scale_y(1.0f), scale_z(1.0f) {}

    glm::vec3 Transform::Local2World(const glm::vec3& v) const {
        if constexpr (false) {
            return glm::mat3(this->transform) * v;  // this is equivalent
        }

        return this->rotation * v;
    }

    glm::vec3 Transform::World2Local(const glm::vec3& v) const {
        // this is equivalent but computing the inverse of a matrix is expensive
        if constexpr (false) {
            return glm::inverse(glm::mat3(this->transform)) * v;
        }

        // this is equivalent iff the matrix is orthogonal (without non-uniform scaling)
        if constexpr (false) {
            return glm::transpose(glm::mat3(this->transform)) * v;
        }

        // the inverse of a quaternion only takes a `glm::dot(vec4, vec4)` so is cheap
        return glm::inverse(this->rotation) * v;
    }

    void Transform::Translate(const glm::vec3& vector, Space space) {
        // local space translation: expect vector in local space coordinates
        if (space == Space::Local) {
            glm::vec3 world_v = Local2World(vector);
            this->position += vector;
            this->transform[3] = glm::vec4(position, 1.0f);
        }
        // world space translation: position is directly updated by the vector
        else {
            this->position += vector;
            this->transform[3] = glm::vec4(position, 1.0f);
        }
    }

    void Transform::Translate(float x, float y, float z, Space space) {
        Translate(glm::vec3(x, y, z), space);
    }

    void Transform::Rotate(const glm::vec3& axis, float angle, Space space) {
        float radians = glm::radians(angle);
        glm::vec3 v = glm::normalize(axis);

        glm::mat4 R = glm::rotate(radians, v);     // rotation matrix4x4
        glm::quat Q = glm::angleAxis(radians, v);  // rotation quaternion

        // local space rotation: expect v in local space, e.g. right = vec3(1, 0, 0)
        if (space == Space::Local) {
            this->transform = this->transform * R;
            this->rotation = glm::normalize(this->rotation * Q);
        }
        // world space rotation: expect v in world space, may introduce translation
        else {
            this->transform = R * this->transform;
            this->rotation = glm::normalize(Q * this->rotation);
            this->position = glm::vec3(this->transform[3]);
        }

        RecalculateEuler();
        RecalculateBasis();
    }

    void Transform::Rotate(const glm::vec3& eulers, Space space) {
        glm::vec3 radians = glm::radians(eulers);

        // rotation matrix4x4
        glm::mat4 RX = glm::rotate(radians.x, world_right);
        glm::mat4 RY = glm::rotate(radians.y, world_up);
        glm::mat4 RZ = glm::rotate(radians.z, world_forward);
        glm::mat4 R = RZ * RX * RY;  // Y->X->Z

        // rotation quaternion
        glm::quat QX = glm::angleAxis(radians.x, world_right);
        glm::quat QY = glm::angleAxis(radians.y, world_up);
        glm::quat QZ = glm::angleAxis(radians.z, world_forward);
        glm::quat Q = QZ * QX * QY;  // Y->X->Z

        // local space rotation: euler angles are relative to local basis vectors
        if (space == Space::Local) {
            this->transform = this->transform * R;
            this->rotation = glm::normalize(this->rotation * Q);
        }
        // world space rotation: euler angles are relative to world basis X, Y and -Z
        else {
            this->transform = R * this->transform;
            this->rotation = glm::normalize(Q * this->rotation);
            this->position = glm::vec3(this->transform[3]);
        }

        RecalculateEuler();
        RecalculateBasis();
    }

    void Transform::Rotate(float euler_x, float euler_y, float euler_z, Space space) {
        Rotate(glm::vec3(euler_x, euler_y, euler_z), space);
    }

    void Transform::Scale(float scale) {
        this->transform = glm::scale(this->transform, glm::vec3(scale));
        this->scale_x *= scale;
        this->scale_y *= scale;
        this->scale_z *= scale;
    }

    void Transform::Scale(const glm::vec3& scale) {
        this->transform = glm::scale(this->transform, scale);
        this->scale_x *= scale.x;
        this->scale_y *= scale.y;
        this->scale_z *= scale.z;
    }

    void Transform::Scale(float scale_x, float scale_y, float scale_z) {
        Scale(glm::vec3(scale_x, scale_y, scale_z));
    }

    void Transform::SetPosition(const glm::vec3& position) {
        this->position = position;
        this->transform[3] = glm::vec4(position, 1.0f);
    }

    void Transform::SetRotation(const glm::quat& rotation) {
        /* the 4 x 4 transform matrix is stored in column-major order as shown below where
           the translation and scaling components are denoted T and S on axis X, Y and Z,
           the rotation component consists of R, U and F for rightward, upward and forward

           [ SX * RX,  SY * UX,  SZ * FX,  TX ]
           [ SX * RY,  SY * UY,  SZ * FY,  TY ]
           [ SX * RZ,  SY * UZ,  SZ * FZ,  TZ ]
           [ 0      ,  0      ,  0      ,  1  ]

           to set rotation directly (absolute change), we construct the matrix from scratch:
           first make an identity matrix, then apply T,S and the new R component back to it.
           note that order matters: we must scale first, then rotate, and finally translate.
        */

        glm::vec4 T = this->transform[3];  // cache the translation component

        // scale first on the identity matrix
        this->transform = glm::mat4(1.0f);
        this->transform[0][0] = scale_x;
        this->transform[1][1] = scale_y;
        this->transform[2][2] = scale_z;
        this->transform[3][3] = 1;

        // [ SX, 00, 00, 00 ] -> now our matrix looks like this
        // [ 00, SY, 00, 00 ]
        // [ 00, 00, SZ, 00 ]
        // [ 00, 00, 00, 01 ]

        // convert the quaternion to a new rotation matrix and apply it back
        this->rotation = glm::normalize(rotation);
        this->transform = glm::mat4_cast(this->rotation) * this->transform;  // world space

        // [ SX * RX,  SY * UX,  SZ * FX,  0 ] -> now our matrix looks like this
        // [ SX * RY,  SY * UY,  SZ * FY,  0 ]
        // [ SX * RZ,  SY * UZ,  SZ * FZ,  0 ]
        // [ 0      ,  0      ,  0      ,  1 ]

        this->transform[3] = T;  // finally apply translation back (replace the last column)

        RecalculateEuler();
        RecalculateBasis();
    }

    void Transform::SetTransform(const glm::mat4& transform) {
        this->scale_x = glm::length(transform[0]);
        this->scale_y = glm::length(transform[1]);
        this->scale_z = glm::length(transform[2]);

        glm::mat3 pure_rotation_matrix = glm::mat3(
            transform[0] / scale_x,  // glm::normalize
            transform[1] / scale_y,  // glm::normalize
            transform[2] / scale_z   // glm::normalize
        );

        this->transform = transform;
        this->position = glm::vec3(transform[3]);
        this->rotation = glm::normalize(glm::quat_cast(pure_rotation_matrix));

        RecalculateEuler();
        RecalculateBasis();
    }

    void Transform::RecalculateBasis() {
        // we can obtain the basis vectors directly from our matrix's first 3 columns.
        // based on our interpretation of basis vectors (see documentation in header),
        // columns 0, 1 and 2 correspond to right, up and backward (in world space).

        this->right   = glm::normalize(glm::vec3(this->transform[0]));
        this->up      = glm::normalize(glm::vec3(this->transform[1]));
        this->forward = glm::normalize(glm::vec3(this->transform[2])) * (-1.0f);

        // another cheap and robust solution is to apply quaternion on the world basis
        if constexpr (false) {
            this->right   = this->rotation * world_right;
            this->up      = this->rotation * world_up;
            this->forward = this->rotation * world_forward;
        }

        // __DO NOT__ use euler angles or cross products which can lead to ambiguity
    }

    void Transform::RecalculateEuler() {
        // extract euler angles from the matrix, in the order of Y->X->Z (yawn->pitch->roll)
        glm::extractEulerAngleYXZ(this->transform, euler_y, euler_x, euler_z);
        this->euler_x = glm::degrees(euler_x);
        this->euler_y = glm::degrees(euler_y);
        this->euler_z = glm::degrees(euler_z);

        // alternatively, we can extract euler angles from our quaternion as follows and it
        // still works. However, be aware that this is not equivalent to the above because
        // the order matters when it comes to euler angles. In particular, `glm::eulerAngles`
        // assumes an implicit order of X->Y->Z, not our Y->X->Z convention, so the values
        // returned are different. In fact both values are correct, they are just different
        // ways to represent the same orientation. It doesn't matter which axis we'd like to
        // rotate around first or last, as long as we are consistent it will be fine, but
        // there's no harm in being explicit.

        if constexpr (false) {
            glm::vec3 eulers = glm::eulerAngles(this->rotation);
            this->euler_x = glm::degrees(eulers.x);
            this->euler_y = glm::degrees(eulers.y);
            this->euler_z = glm::degrees(eulers.z);
        }
    }

}
