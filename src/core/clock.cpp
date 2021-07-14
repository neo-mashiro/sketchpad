#include "pch.h"
#include "clock.h"

namespace core {

    decltype(Clock::start_time) Clock::start_time = std::chrono::high_resolution_clock::now();

    void Clock::Reset() {
        start_time = std::chrono::high_resolution_clock::now();
        last_frame = this_frame = delta_time = time = 0.0f;
    }

    void Clock::Update() {
        this_frame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        delta_time = this_frame - last_frame;
        last_frame = this_frame;
        time += delta_time;
    }
}
