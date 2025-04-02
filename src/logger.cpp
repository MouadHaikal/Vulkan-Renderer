#include "logger.hpp"
#include "utilities.hpp"

#include <cctype>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vulkan/vulkan_core.h>

#define ICON_SUCCESS "\033[32m\u2713\033[0m "
#define ICON_WARNING "\033[33m\u26A0\033[0m "
#define ICON_FAILURE "\033[31m\u274C\033[0m"

// Innitialize static members
Logger*    Logger::instance = nullptr;
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

Logger::Level Logger::getMinLevel(){ return minLevel; }

void Logger::setMinLevel(Level level){ 
    std::lock_guard<std::mutex> lock(logMutex);
    minLevel = level; 
}

void Logger::log(Level level, const std::string& message) const{
    if (level < minLevel) return;

    std::lock_guard<std::mutex> lock(logMutex);

    std::ostream& stream = (level >= Level::ERROR)? std::cerr : std::cout;
    stream << "[" << levelToString(level) << "] " << message << std::endl;

    if (level == Level::FATAL) {
        stream << "Aborting" << std::endl;
        std::abort();    
    }
}

void Logger::logResult(VkResult result, const std::string& operation, const logFlags& flags) const{
    if (result == VK_SUCCESS) {
        if (flags.traceSuccess) {
            log(Level::TRACE, ICON_SUCCESS + operation);
        }

    } else{
        std::string icon = (flags.failureLevel >= Level::ERROR) ? ICON_FAILURE : ICON_WARNING;
        log(flags.failureLevel, icon + "Failed to " + std::string(1, static_cast<char>(tolower(operation[0]))) + operation.substr(1));
    }
}

void Logger::logDeviceInfo(VkPhysicalDevice device, QueueFamilyIndices queueFamilies) const{
    if (minLevel > Level::INFO) return;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    LOG_INFO("================== Device Info ==================");

    LOG_INFO_S("- Name : " << deviceProperties.deviceName);
    switch (deviceProperties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            LOG_INFO("- Type : Integrated GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            LOG_INFO("- Type : Discrete GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            LOG_INFO("- Type : Virtual GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            LOG_INFO("- Type : CPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            LOG_INFO("- Type : Other");
            break;
        default: ;
    }
    LOG_DEBUG("- Queue family indices :");
        LOG_DEBUG_S("\tGraphics : " << queueFamilies.graphicsFamily.value());
        LOG_DEBUG_S("\tPresent  : " << queueFamilies.presentFamily.value());
        LOG_DEBUG_S("\tTransfer : " << queueFamilies.transferFamily.value());

    LOG_INFO("=================================================");
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
