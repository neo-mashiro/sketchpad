#include "pch.h"

#include <GL/freeglut.h>
#include <GLFW/glfw3.h>
#include <date/date.h>
#include "core/clock.h"

namespace core {

    decltype(Clock::start_time) Clock::start_time = std::chrono::system_clock::now();

    #pragma warning(push)
    #pragma warning(disable : 4244)

    std::string Clock::GetDateTimeUTC() {
        namespace chrono = std::chrono;

        // keep sync with glut/glfw internal clock
        auto now = start_time + chrono::seconds((int)time);
        auto sysdate = date::floor<date::days>(now);
        auto systime = date::make_time(chrono::duration_cast<chrono::seconds>(now - sysdate));

        int hour   = systime.hours().count();
        int minute = systime.minutes().count();
        int second = systime.seconds().count();

        // append '0' to single-digit numbers
        static auto format = [](int i) {
            std::ostringstream sstr;
            sstr << std::setw(2) << std::setfill('0') << i;
            return sstr.str();
        };

        std::string YYYY_MM_DD = date::format("%Y-%m-%d", sysdate);
        std::string HH24_MM_SS = format(hour) + format(minute) + format(second);

        return YYYY_MM_DD + "-" + HH24_MM_SS;
    }

    #pragma warning(pop)

    void Clock::Reset() {
        start_time = std::chrono::system_clock::now();
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

        // for devices that tick at a fixed interval (e.g. timers and stopwatches), it's easier
        // to work with delta time, but this approach suffers from floating point imprecision.
        // when the elapsed time becomes a very large float (99:59:59 requires 359,999 seconds),
        // and you add or subtract very small numbers (delta time) from it over and over again,
        // rounding errors can accumulate over time and lead to unexpected result.

        // it is possible to partially alleviate this issue by using double instead of float, so
        // that we have more precision, more resolution and less rounding errors, but still to a
        // limited extent. For robustness, we must compare real time to a fixed start timestamp.

        if constexpr (true) {
            time = this_frame;
        }
        else {
            time += delta_time;  // never do this !!! watch out for rounding errors...
        }

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
