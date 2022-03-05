#pragma once

#include <glm/glm.hpp>
#include "component/component.h"

namespace component {

    /* this is a simple implementation of the transform component which supports translation,
       rotation and scaling, other types of transformation such as reflection or shearing or
       negative scaling are currently not supported, but you can easily extend the class.

       when working with the transform component, it should be noted that affine transforms
       may not preserve orthogonality. Keep in mind that rotations and translation are always
       orthogonal, wheras non-uniform scaling or shearing is not. If the transform matrix is
       not orthogonal, we cannot transform a normal vector by simply multiplying it with the
       matrix itself, but have to use the transpose of the inverse of it, this also applies
       to other vectors like tangents. If you really need transformation to be very complex,
       it's probably better to write up a geometry shader and transfer the complexity there.

       Translation:
       --------------------------------------------------------------------------------------
       our translate function expects a translation vector whose amount is measured in world
       units, this amount does not take account of the current scaling factor.

       Scaling:
       --------------------------------------------------------------------------------------
       negative scaling is not allowed, each scaling factor must be > 0. However, non-uniform
       scaling is supported, but you need to use it with care. Keep in mind that non-uniform
       scaling will break the orthogonality of the model matrix, so you have to calculate the
       transpose of its inverse when transforming vertices and vectors. Also, while uniform
       scaling and rotation are guaranteed to commute so they can be applied in an arbitrary
       order, non-uniform scaling does not have this property so be aware of the order.

       Rotation:
       --------------------------------------------------------------------------------------
       our rotation is always local rotation, it's not relative to the world space. Also, the
       rotate function expects all angle to be measured in degrees. For interpolation between
       rotations, check out the `Slerp()` function from the math utility namespace.

       for robustness, internally we adopt a quaternion representation of rotations, all the
       computation is done based on quaternions. Externally, the rotate functions are exposed
       to the user as either euler angle or rotation axis, while most of time we are going to
       use a rotation axis, be aware that working with euler angles is evil. Here's a couple
       of things to bear in mind when working with rotations:

       > rotations in 3D typically DO NOT commute (regardless of representation)
       > order of rotation is important for euler angles (need a convention like XYZ or YXZ)
       > people use different convention for euler angles and there's no universal standard
       > using trigonometric functions implicitly rely on the convention of euler angles
       > notice the difference between "rotation" and "orientation" (not the same)
       > notice the difference between "local space" vs "world space"

       Euler Angles:
       --------------------------------------------------------------------------------------
       our convention with euler angles is that, a positive angle always corresponds to a
       counterclockwise rotation about an axis, and the rotations are applied in the order of
       yawn->pitch->roll (or y->x->z). Ideally, users should limit the range of euler angles
       to avoid ambiguity, so that pitch, yawn and roll are always in the (-180, 180) range.
       in this demo, euler angles are primarily used by our camera class because camera needs
       to be clamped on the vertical axis. Otherwise, using a rotation axis is preferred.

       Quaternion:
       --------------------------------------------------------------------------------------
       a unit quaternion represents a rotation in 3D space, a non-unit quaternion does not.
       a unit quaternion has norm 1, and the norm of quaternion is multiplicative, that is:
       > norm(p * q) = norm(p) * norm(q), for any quaternions p and q.
       this implies that multiplication of quaternions preserves the norm, but we need to be
       aware of floating point precision errors, which if accumulated could lead to problems.

       Local vs World:
       --------------------------------------------------------------------------------------
       confusion can arise if the user wasn't aware of the reference frame he's working with,
       for translation and rotation, it's important to know what basis we are referring to,
       local space or world space. For instance, a vector will have different coordinates in
       local space vs world space, this is basically a "change-of-basis" problem.

       in math, world space operations are calculated directly, but for local operations, the
       solution is to transform to world origin or zero orientation first, apply the change,
       and finally transform back. What this implies is that, when multiplying matrices and
       quaternions in glm, local operations must be applied on the right, whereas world space
       operations must be applied on the left (relative to the current matrix or quaternion).

       note that an axis vector alone isn't enough to uniquely define a world space rotation,
       an additional "pivot" parameter is required. For instance, (0,1,0) is just one of the
       possible vectors that can represent world up, there's an infinite number of equivalent
       world up vectors. However, a world space rotation around each one of them is totally
       different, thus we need a pivot point to tell which vector to use.
       
       for simplicity, we will only consider the order of multiplication and not bother with
       the pivot point, so when we say a world space rotation around vector v, it is really
       the rotation around vector w, where w is v translated such that it passes through the
       world origin. If using the pivot, we'd need to translate by `-pivot` first, then apply
       the rotation, and finally translate back by `+pivot`.

       Set Functions:
       --------------------------------------------------------------------------------------
       the set functions are used to set position, rotation or transform directly, which are
       useful in case we need to transform a model in world space or interpolate position and
       rotation between frames (often used in pair with lerp/slerp + a framerate-independent
       ease factor), or trying to update the transform component using an interactive gizmo
       from the UI. Unlike translate, rotate and scale which apply incremental change on the
       transform, the "set" functions overwrite the current transform by setting absolute new
       values, and the values are always relative to the world space, not local.

       Handedness:
       --------------------------------------------------------------------------------------
       by default, OpenGL adopts a right-handed coordinate system: from one's point of view,
       the backward direction vector is the +z axis, up is +y axis, and right is +x axis. As
       a matter of pure taste, each system may have a different convention of handedness, for
       example, Houdini is also right-handed, but DirectX and Unity use a left-handed system
       where +z translates to forward. On top of that, each system has its own convention of
       world space direction (basis) vectors, for example, Unreal is also left-handed but it
       uses +z as the up vector instead of +y, and a flight simulation system might find it
       more convenient to use +x as up... so there's no universal convention in this regard.

       despite the mess here, it bears noting that handedness isn't a big concern, eventually
       the definition of world space basis vectors is up to the user. While OpenGL is right-
       handed, the NDC space is inherently left-handed, so people commonly use +x for right,
       +y for up and -z for forward, which means that the camera is initially looking down at
       the -z axis. This also aligns with the initial state for `glDepthFunc(GL_LESS)`, so we
       are going to follow the same rule to interpret our basis vectors.

       note that convertion is necessary between systems that adopt different conventions. To
       convert between left and right handedness, `glm::scale(1,1,-1)` will do the trick, or
       if you don't want to change the matrix, you can reverse the front face, or reverse the
       winding order, and possibly need to adjust the depth values as well. To convert the
       interpretation of basis vectors though, we can only swap xyz in the transform matrix.

       Debugging:
       --------------------------------------------------------------------------------------
       when it comes to debugging the transform, a lot of test data is often required for the
       math part. This task is challenging because many vectors, matrices and quaternions are
       getting involved in the process, and we have to ensure the mathematical correctness of
       every single number, possibly with just pens and papers. Hand calculation is not only
       hard (especially for rotations), but also tedious, time-consuming and error-prone when
       we need a large amount of test cases, so definitely should be avoided.

       to alleviate this issue, automatic tools should be used instead. Matlab or Mathematica
       seems like overkill, but there are also many small calculators online, check out this:
       https://www.andre-gaschler.com/rotationconverter/

       in addition, the glm library extension has a helper function `glm::to_string()`, which
       can be used to print all vectors, matrices and quaternions in human-readable format.
    */

    enum class Space : char {
        Local = 1 << 0,
        World = 1 << 1
    };

    class Transform : public Component {
      private:
        void RecalculateBasis(void);
        void RecalculateEuler(void);

      public:
        glm::vec3 position;
        glm::quat rotation;   // rotations are internally represented as quaternions
        glm::mat4 transform;  // 4x4 homogeneous matrix stored in column-major order

        float euler_x, euler_y, euler_z;  // euler angles in degrees (pitch, yawn, roll)
        float scale_x, scale_y, scale_z;

        glm::vec3 up;
        glm::vec3 forward;
        glm::vec3 right;

        Transform();

        glm::vec3 Local2World(const glm::vec3& v) const;
        glm::vec3 World2Local(const glm::vec3& v) const;

        void Translate(const glm::vec3& vector, Space space = Space::World);
        void Translate(float x, float y, float z, Space space = Space::World);

        void Rotate(const glm::vec3& axis, float angle, Space space);
        void Rotate(const glm::vec3& eulers, Space space);
        void Rotate(float euler_x, float euler_y, float euler_z, Space space);

        void Scale(float scale);
        void Scale(const glm::vec3& scale);
        void Scale(float scale_x, float scale_y, float scale_z);

        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::quat& rotation);
        void SetTransform(const glm::mat4& transform);
    };

}