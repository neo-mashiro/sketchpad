#include "pch.h"

#include "core/input.h"
#include "core/clock.h"
#include "core/window.h"
#include "component/camera.h"
#include "component/transform.h"
#include "utils/math.h"

using namespace core;
using namespace utils::math;

namespace component {

    Camera::Camera(Transform* T, View view) :
        Component(),
        fov(90.0f),
        near_clip(0.1f),
        far_clip(100.0f),
        move_speed(5.0f),
        zoom_speed(0.04f),
        rotate_speed(0.3f),
        orbit_speed(0.05f),
        initial_position(T->position),
        initial_rotation(T->rotation),
        T(T), view(view) {}

    glm::mat4 Camera::GetViewMatrix() const {
        return glm::lookAt(T->position, T->position + T->forward, T->up);
    }

    glm::mat4 Camera::GetProjectionMatrix() const {
        return (view == View::Orthgraphic)
            ? glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_clip, far_clip)
            : glm::perspective(glm::radians(fov), Window::aspect_ratio, near_clip, far_clip);
    }

    void Camera::Update() {
        float deltatime = Clock::delta_time;
        static bool recovering = false;

        // smoothly recover camera to its initial position and orientation
        if (recovering) {
            float t = EaseFactor(10.0f, deltatime);
            T->SetPosition(Lerp(T->position, initial_position, t));
            T->SetRotation(SlerpRaw(T->rotation, initial_rotation, t));

            if (Equals(T->position, initial_position) && Equals(T->rotation, initial_rotation)) {
                recovering = false;  // keep recovering until both position and rotation are recovered
            }

            return;  // all user inputs are ignored during recovery transition
        }

        // camera orbit: move cursor around while holding the left button (arcball camera)
        if (Input::GetMouseDown(MouseButton::Left)) {
            static constexpr auto world_up = glm::vec3(0.0f, 1.0f, 0.0f);

            // for arcball mode, horizontal orbit is around the world up axis (+Y), vertical
            // orbit is about the local right vector, both rotations are done in world space
            // vertical orbit should be clamped to a range s.t. camera's euler angle x can
            // never escape the (-90, 90) bound in degrees, o/w the world will be inverted

            float orbit_y = -Input::GetCursorOffset(MouseAxis::Horizontal) * orbit_speed;
            float orbit_x = -Input::GetCursorOffset(MouseAxis::Vertical) * orbit_speed;

            // clamp `T->euler_x + orbit_x` to (-89, 89)
            orbit_x = glm::clamp(orbit_x, -T->euler_x - 89.0f, -T->euler_x + 89.0f);

            T->Rotate(world_up, orbit_y, Space::World);
            T->Rotate(T->right, orbit_x, Space::World);
            return;  // ignore other events (keys) in arcball mode
        }

        // camera zoom: slide the cursor while holding the right button
        if (Input::GetMouseDown(MouseButton::Right)) {
            fov -= Input::GetCursorOffset(MouseAxis::Horizontal) * zoom_speed;
            fov = glm::clamp(fov, 30.0f, 120.0f);
            return;  // ignore other events (keys and rotation) during smooth zooming
        }

        // once the mouse buttons are released, smoothly lerp fov back to 90 degrees (default)
        fov = Lerp(fov, 90.0f, EaseFactor(20.0f, deltatime));

        // key events are only processed if there's no mouse button event (which cannot be interrupted)
        if (Input::GetKeyDown('r')) {
            recovering = true;
        }

        // rotation is limited to the X and Y axis (pitch and yawn only, no roll)
        float euler_y = T->euler_y - Input::GetCursorOffset(MouseAxis::Horizontal) * rotate_speed;
        float euler_x = T->euler_x - Input::GetCursorOffset(MouseAxis::Vertical) * rotate_speed;

        euler_y = glm::radians(euler_y);
        euler_x = glm::radians(glm::clamp(euler_x, -89.0f, 89.0f));  // clamp vertical rotation
        glm::quat rotation = glm::quat_cast(glm::eulerAngleYXZ(euler_y, euler_x, 0.0f));

        T->SetRotation(rotation);

        // translation (not normalized, movement is faster along the diagonal)
        if (Input::GetKeyDown('w')) {
            T->Translate(T->forward * (move_speed * deltatime));
        }

        if (Input::GetKeyDown('s')) {
            T->Translate(-T->forward * (move_speed * deltatime));
        }

        if (Input::GetKeyDown('a')) {
            T->Translate(-T->right * (move_speed * deltatime));
        }

        if (Input::GetKeyDown('d')) {
            T->Translate(T->right * (move_speed * deltatime));
        }

        if (Input::GetKeyDown('z')) {
            T->Translate(-T->up * (move_speed * deltatime));
        }

        if (Input::GetKeyDown(0x20)) {  // VK_SPACE
            T->Translate(T->up * (move_speed * deltatime));
        }
    }

}
