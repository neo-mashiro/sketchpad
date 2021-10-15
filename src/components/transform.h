#pragma once

#include <glm/glm.hpp>
#include "components/component.h"

namespace components {

    /* this is a simple implementation of the transform component which supports translation,
       rotation and scaling, other types of transformation such as reflection or shearing is
       currently not supported, but you can easily extend the class.

       when working with the transform component, it should be noted that affine transforms
       may not preserve orthogonality. Keep in mind that rotations and translation are always
       orthogonal, wheras non-uniform scaling or shearing is not. If the transform matrix is
       not orthogonal, we cannot transform a normal vector by simply multiplying it with the
       matrix itself, but have to use the transpose of the inverse of it, this also applies
       to other vectors like tangents. If you really need transformation to be very complex,
       it's probably better to write up a geometry shader.

       Translation:
       --------------------------------------------------------------------------------------
       our translation function always expects the vector to be measured in absolute amount,
       this amount will not affected by the current scaling factor, even if it's not 1.

       Scaling:
       --------------------------------------------------------------------------------------
       in this class, we will only allow uniform scaling so as to keep the transform matrix
       orthogonal, this way, it's easier to transform a normal vector (just multiply by the
       transform), there's no need to calculate the transpose of its inverse. Another benefit
       of doing so is that, uniform scaling and rotation are guaranteed to commute, so we are
       able to apply them in an arbitrary order.

       Rotation:
       --------------------------------------------------------------------------------------
       to keep it simple, our rotation is not based on quaternions and will not support smooth
       interpolation between rotations (will add this later). Instead, we are going to adopt
       the matrix representation of rotations. The basis vectors are also recalculated every
       frame based on the rotation matrix, this is still more robust than using trigonometric
       functions, which rely on the conventions of euler angles (BAD).

       here we have provided 2 rotation functions for the user, one rotation is around a given
       axis (incremental change applied on the current rotation), the other one is given by a
       vector of euler angles in degrees (absolute change relative to 0 orientation). However,
       keep in mind that euler angles are evil, whenever possible, you should use the first
       function, the second one is intended for setting rotations directly (absolute change),
       such as when you want to set an object's initial orientation. In our demo, the second
       one is primarily used by our camera class because camera rotation needs to be clamped
       on the vertical axis, for which euler angles are easier to work with. (I have no idea
       how to clamp a matrix or quaternion......)

       using the second rotation function could be dangerous, because rotations in 3D usually
       do not commute. On top of that, unlike matrices or quaternions, people like to use
       different conventions for euler angles and they are not unique, for example, (0, 90, 0)
       and (-180, 90, 180) could represent the same rotation if you follow a specific order.
       our convention with euler angles is that, a positive angle always corresponds to a
       counter-clockwise rotation about an axis, and the rotations are applied in the order of
       yawn->pitch->roll (or y->x->z). Also, users should limit the range of angles to avoid
       ambiguity, which must work with the glm functions being used here. Since the order of
       rotation is y->x->z, the middle one is pitch, pitch should be between (-90, 90) degrees
       and yawn and roll should be within (-180, 180).
    */

    class Transform : public Component {
      private:
        void RecalculateBasis(void);

      public:
        Transform();
        ~Transform() {}

        Transform(const Transform&) = delete;
        Transform& operator=(const Transform&) = delete;

        Transform(Transform&& other) = default;
        Transform& operator=(Transform&& other) = default;

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
