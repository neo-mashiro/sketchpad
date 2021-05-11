#pragma once

#include "canvas.h"
#include "spdlog/spdlog.h"

#ifdef _DEBUG
    #define CORE_INFO(...)  ::Sketchpad::Log::GetLogger()->info(__VA_ARGS__)
    #define CORE_WARN(...)  ::Sketchpad::Log::GetLogger()->warn(__VA_ARGS__)
    #define CORE_ERROR(...) ::Sketchpad::Log::GetLogger()->error(__VA_ARGS__)
#else
    #define CORE_INFO
    #define CORE_WARN
    #define CORE_ERROR
#endif

namespace Sketchpad {

    class Log {
      private:
        static std::shared_ptr<spdlog::logger> logger;

	  public:
		static void Init();
        static std::shared_ptr<spdlog::logger> GetLogger() { return logger; }
	};
}
