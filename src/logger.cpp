#include "logger.hpp"

#include <iostream>

// Innitialize static members
Logger* Logger::instance = nullptr;
std::mutex Logger::instanceMutex;
std::mutex Logger::logMutex;


Logger& Logger::get(){
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance == nullptr) {
        instance = new Logger();
    }
    return *instance;
}

void Logger::destroy(){
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

void Logger::setMinLevel(Level level){ 
    std::lock_guard<std::mutex> lock(logMutex);
    minLevel = level; 
}

void Logger::log(Level level, const std::string& message) const{
    std::lock_guard<std::mutex> lock(logMutex);

    if (level < minLevel) return;

    std::ostream& stream = (level >= Level::ERROR)? std::cerr : std::cout;
    stream << "[" << levelToString(level) << "] " << message << std::endl;
}

void Logger::logResult(VkResult result, const std::string& operation) const{
    if (result != VK_SUCCESS) {
        log(Level::ERROR, operation + " failed!");
    } else if (minLevel <= Level::DEBUG) {
        log(Level::DEBUG, operation + " succeeded");
    }
}

const char* Logger::levelToString(Level level) const{
    switch (level) {
        case Level::TRACE  : return "TRACE";
        case Level::DEBUG  : return "DEBUG";
        case Level::INFO   : return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR  : return "ERROR";
        case Level::FATAL  : return "FATAL";

        default            : return "UNKNOWN";
    }
}
