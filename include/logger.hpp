#pragma once

#include <vulkan/vulkan_core.h>

#include "utilities.hpp"

#include <sstream>    // Indirectly used in macros
#include <cmath>
#include <mutex>
#include <string>


class Logger{
public:
    enum class Level {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    struct logFlags{
        bool traceSuccess;
        Level failureLevel;

        logFlags(bool traceSuccess = false, Level failureLevel = Level::ERROR): 
            traceSuccess(traceSuccess), failureLevel(failureLevel){}
    };


    static Logger& get();
    static void destroy();


    Level getMinLevel();
    void setMinLevel(Level level);

    void log(Level level, const std::string& message) const;
    void logResult(VkResult result, const std::string& operation, const logFlags& flags) const;
    void logDeviceInfo(VkPhysicalDevice device, QueueFamilyIndices queueFamilies) const;


    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(Logger&&)      = delete;

private:
    Logger()  = default;
    ~Logger() = default;


    static Logger*    instance;
    static std::mutex instanceMutex;
    static std::mutex logMutex;

    Level             minLevel = Level::INFO;


    const char* levelToString(Level level) const;
    std::string lowerCase(const std::string& str) const;
};


#define LOG_TRACE(msg)   Logger::get().log(Logger::Level::TRACE  , msg)
#define LOG_DEBUG(msg)   Logger::get().log(Logger::Level::DEBUG  , msg)
#define LOG_INFO(msg)    Logger::get().log(Logger::Level::INFO   , msg)
#define LOG_WARNING(msg) Logger::get().log(Logger::Level::WARNING, msg)
#define LOG_ERROR(msg)   Logger::get().log(Logger::Level::ERROR  , msg)
#define LOG_FATAL(msg)   Logger::get().log(Logger::Level::FATAL  , msg)

#define LOG_RESULT(result, operation)        Logger::get().logResult(result, operation, Logger::logFlags(true , Logger::Level::FATAL))
#define LOG_RESULT_SILENT(result, operation) Logger::get().logResult(result, operation, Logger::logFlags(false, Logger::Level::FATAL))
#define LOG_RESULT_OPT(result, operation)    Logger::get().logResult(result, operation, Logger::logFlags(true , Logger::Level::ERROR))

// Can/Should only be used inside Renderer class - findQueueFamilies() function is defined in Renderer class
#define LOG_DEVICE_INFO(device)              Logger::get().logDeviceInfo(device, findQueueFamilies(device));  

#define LOG_TRACE_S(stream) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        Logger::get().log(Logger::Level::TRACE, oss__.str()); \
    } while(0) 

#define LOG_DEBUG_S(stream) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        Logger::get().log(Logger::Level::DEBUG, oss__.str()); \
    } while(0) 

#define LOG_INFO_S(stream) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        Logger::get().log(Logger::Level::INFO, oss__.str()); \
    } while(0) 

#define LOG_WARNING_S(stream) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        Logger::get().log(Logger::Level::WARNING, oss__.str()); \
    } while(0) 

#define LOG_ERROR_S(stream) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        Logger::get().log(Logger::Level::ERROR, oss__.str()); \
    } while(0) 

#define LOG_FATAL_S(stream) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        Logger::get().log(Logger::Level::FATAL, oss__.str()); \
    } while(0) 
