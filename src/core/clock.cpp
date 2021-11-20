#include "pch.h"

#include <GL/freeglut.h>
#include <GLFW/glfw3.h>
#include "core/clock.h"

namespace core {

    decltype(Clock::start_time) Clock::start_time = std::chrono::high_resolution_clock::now();

    void Clock::Reset() {
        start_time = std::chrono::high_resolution_clock::now();
        last_frame = this_frame = delta_time = time = 0.0f;

        fps = ms = 0.0f;
        frame_count = 0;
        duration = 0.0f;
    }

    void Clock::Update() {
        if constexpr (_freeglut) {
            this_frame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        }
        else {
            this_frame = static_cast<float>(glfwGetTime());
        }
        
        delta_time = this_frame - last_frame;
        last_frame = this_frame;
        time += delta_time;

        // compute frames per second
        frame_count++;
        duration += delta_time;

        if (duration >= 0.1f) {
            fps = frame_count / duration;
            ms = 1000.0f * duration / frame_count;
            frame_count = 0;
            duration = 0.0f;
        }
    }
}
