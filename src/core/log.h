#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#ifdef _DEBUG
    #define CORE_INFO(...)  ::core::Log::GetLogger()->info(__VA_ARGS__)
    #define CORE_WARN(...)  ::core::Log::GetLogger()->warn(__VA_ARGS__)
    #define CORE_ERROR(...) ::core::Log::GetLogger()->error(__VA_ARGS__)
#else
    #define CORE_INFO
    #define CORE_WARN
    #define CORE_ERROR
#endif

namespace core {

    class Log {
      private:
        static std::shared_ptr<spdlog::logger> logger;

      public:
        static void Init();
        static std::shared_ptr<spdlog::logger> GetLogger() { return logger; }
    };
}
