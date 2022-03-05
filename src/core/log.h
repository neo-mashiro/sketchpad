#pragma once

#include <spdlog/spdlog.h>
#include "core/base.h"

#define CORE_INFO(...)  ::core::Log::GetLogger()->info(__VA_ARGS__)
#define CORE_WARN(...)  ::core::Log::GetLogger()->warn(__VA_ARGS__)
#define CORE_DEBUG(...) ::core::Log::GetLogger()->debug(__VA_ARGS__)
#define CORE_TRACE(...) ::core::Log::GetLogger()->trace(__VA_ARGS__)
#define CORE_ERROR(...) ::core::Log::GetLogger()->error(__VA_ARGS__)

// exceptions, error handling and varying levels of logging is part of the app's normal workflow
// assertions however, must hold in any correct release build, they only apply to the debug mode

#if defined(_DEBUG) || defined(DEBUG)
    #define CORE_ASERT(cond, ...) { if (!(cond)) { ::core::Log::GetLogger()->critical(__VA_ARGS__); SP_DBG_BREAK(); } }
#else
    #define CORE_ASERT(cond, ...)
#endif

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
