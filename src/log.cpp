#include "log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Sketchpad {

    // each sink writes the log to a single target (console, file, database, etc.)
    // each sink has its own private instance of formatter object
    // each logger contains a vector of one or more `std::shared_ptr<sink>`
    // the logger is maintained in a global registry, which can be accessed easily from anywhere
    // on a log call, the logger calls each sink to log message to their respective targets

	std::shared_ptr<spdlog::logger> Log::logger;

	void Log::Init() {
		std::vector<spdlog::sink_ptr> sinks;

		// emplace back more sinks if you need
		sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());  // console sink

		// set a separate formatter for each sink
		sinks[0]->set_pattern("%^%T > [%l] %v%$");  // 23:55:59 > [info] sample message

		// create the logger from our sinks and register it in global registry
		logger = std::make_shared<spdlog::logger>("sketchpad", begin(sinks), end(sinks));
		spdlog::register_logger(logger);

		// set log level (threshold) for the logger
		logger->set_level(spdlog::level::trace);  // log level less than this will be silently ignored
		logger->flush_on(spdlog::level::trace);   // the minimum log level that will trigger automatic flush
	}
}
