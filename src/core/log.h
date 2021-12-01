#pragma once

#include <spdlog/spdlog.h>

#define CORE_INFO(...)  ::core::Log::GetLogger()->info(__VA_ARGS__)
#define CORE_WARN(...)  ::core::Log::GetLogger()->warn(__VA_ARGS__)
#define CORE_TRACE(...) ::core::Log::GetLogger()->trace(__VA_ARGS__)
#define CORE_ERROR(...) ::core::Log::GetLogger()->error(__VA_ARGS__)
#define CORE_ASERT(cond, ...) { if (!(cond)) { ::core::Log::GetLogger()->critical(__VA_ARGS__); __debugbreak(); } }

namespace core {

    class Log {
      private:
        static std::shared_ptr<spdlog::logger> logger;

      public:
        static std::shared_ptr<spdlog::logger> GetLogger() { return logger; }

      public:
        static void Init();
        static void Shutdown();
    };

}
