#pragma once

#include "spdlog/spdlog.h"

#ifdef _DEBUG
    #define CORE_INFO(...)  ::core::Log::GetLogger()->info(__VA_ARGS__)
    #define CORE_WARN(...)  ::core::Log::GetLogger()->warn(__VA_ARGS__)
    #define CORE_TRACE(...) ::core::Log::GetLogger()->trace(__VA_ARGS__)
#else
    #define CORE_INFO
    #define CORE_WARN
    #define CORE_TRACE
#endif

#define CORE_ERROR(...) ::core::Log::GetLogger()->error(__VA_ARGS__)
#define CORE_ASERT(cond, ...) { if (!(cond)) { ::core::Log::GetLogger()->critical(__VA_ARGS__); __debugbreak(); } }

// MSVC intrinsic type name inference
#include <string_view>

template<typename T>
constexpr auto type_name() noexcept {
    std::string_view type = __FUNCSIG__;
    type.remove_prefix(std::string_view("auto __cdecl type_name<").size());
    type.remove_suffix(std::string_view(">(void) noexcept").size());
    return type;
}

namespace core {

    class Log {
      private:
        static std::shared_ptr<spdlog::logger> logger;

      public:
        static void Init();
        static std::shared_ptr<spdlog::logger> GetLogger() { return logger; }
    };

}
