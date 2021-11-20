#include "pch.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include "core/log.h"

namespace core {

    // each sink writes the log to a single target (console, file, database, etc.)
    // each sink has its own private instance of formatter object
    // each logger contains a vector of one or more `std::shared_ptr<sink>`
    // the logger is maintained in a global registry, which can be accessed easily from anywhere
    // on a log call, the logger calls each sink to log message to their respective targets

    std::shared_ptr<spdlog::logger> Log::logger;

    /* the logger expects a vector of shared pointers to sinks, so the steps are:
      
       [1] make a std::vector to store the pointers
       [2] emplace back sink pointers to the vector
       [3] for each individual sink in the vector, set format and level (or use default)
       [4] construct the logger from the vector and register it in global registry
       [5] we can also set log level (threshold) for the entire logger (applies to every sink)

       [*] if you only need basic sinks, using `std::vector<spdlog::sink_ptr> sinks` is fine,
           but if you want to set custom colors or work with a custom sink, you should use a
           pointer type of the derived sink class that supports these functions, otherwise
           `sinks[i]` will fallback to the base sink type, which does not have the methods you
           want unless you manually cast them to the right pointer types.
    */

    void Log::Init() {
        using wincolor_sink_ptr = std::shared_ptr<spdlog::sinks::stdout_color_sink_mt>;

        std::vector<wincolor_sink_ptr> sinks;  // pointers to sinks that support setting custom color
        sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());  // console sink
        sinks[0]->set_pattern("%^%T > [%L] %v%$");  // e.g. 23:55:59 > [info] sample message
        sinks[0]->set_color(spdlog::level::trace, sinks[0]->CYAN);

        logger = std::make_shared<spdlog::logger>("sketchpad", begin(sinks), end(sinks));
        spdlog::register_logger(logger);
        logger->set_level(spdlog::level::trace);  // log level less than this will be silently ignored
        logger->flush_on(spdlog::level::trace);   // the minimum log level that will trigger automatic flush
    }

    void Log::CheckGLError(int checkpoint) {
        // to ignore an error and suppress the message, pass a checkpoint of -1
        GLenum err_code;
        while ((err_code = glGetError()) != GL_NO_ERROR) {
            if (err_code != -1) {
                CORE_ERROR("OpenGL error detected at checkpoint {0}: {1}", checkpoint, err_code);
            }
        }
    }
}
