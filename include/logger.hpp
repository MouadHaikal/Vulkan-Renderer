#pragma once

#include <mutex>
#include <string>
#include <vulkan/vulkan_core.h>

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


    static Logger& get();
    static void destroy();


    void setMinLevel(Level level);

    void log(Level level, const std::string& message) const;
    void logResult(VkResult result, const std::string& operation) const;


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
};

#define LOG_TRACE(msg)   Logger::get().log(Logger::Level::TRACE  , msg)
#define LOG_DEBUG(msg)   Logger::get().log(Logger::Level::DEBUG  , msg)
#define LOG_INFO(msg)    Logger::get().log(Logger::Level::INFO   , msg)
#define LOG_WARNING(msg) Logger::get().log(Logger::Level::WARNING, msg)
#define LOG_ERROR(msg)   Logger::get().log(Logger::Level::ERROR  , msg)
#define LOG_FATAL(msg)   Logger::get().log(Logger::Level::FATAL  , msg)

#define LOG_RESULT(result, operation)  Logger::get().logResult(result, operation)
