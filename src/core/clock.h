#pragma once

#include <chrono>

namespace core {

    class Clock {
      private:
        static std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        static inline float last_frame = 0.0f;
        static inline float this_frame = 0.0f;

        // realtime fps is sampled every 0.1 second
        static inline int frame_count = 0;
        static inline float duration = 0.0f;

      public:
        static inline float delta_time = 0.0f;
        static inline float time = 0.0f;

        static inline float fps = 0.0f;  // frames per second
        static inline float ms = 0.0f;   // milliseconds per frame

        static void Reset();
        static void Update();
    };
}
