#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <chrono>

namespace core {

    class Clock {
      private:
        static std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        static inline float last_frame = 0.0f;
        static inline float this_frame = 0.0f;

      public:
        static inline float delta_time = 0.0f;
        static inline float time = 0.0f;

        static void Reset();
        static void Update();
    };
}
