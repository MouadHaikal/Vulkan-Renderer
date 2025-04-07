#include <logger.hpp>
#include <utilities.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

#define TEXT_COLOR_GRAY       "\033[97m"
#define TEXT_COLOR_WHITE      "\033[37m"
#define TEXT_COLOR_WHITE_BOLD "\033[1;37m"
#define TEXT_COLOR_YELLOW     "\033[93m"
#define TEXT_COLOR_ORANGE     "\033[38;5;208m"
#define TEXT_COLOR_FLAME      "\033[38;5;202m"
#define TEXT_COLOR_RED_BOLD   "\033[1;38;5;160m"

#define RESET_TEXT_COLOR      "\033[0m"


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
    stream << levelToString(level) << message << RESET_TEXT_COLOR << std::endl;

    if (level == Level::FATAL) {
        stream << TEXT_COLOR_RED_BOLD "Aborting" RESET_TEXT_COLOR << std::endl;
        std::abort();    
    }
}

void Logger::logResult(VkResult result, const std::string& operation, const logFlags& flags) const{
    if (result == VK_SUCCESS) {
        if (flags.traceSuccess) {
            log(Level::TRACE, operation);
        }

    } else{
        log(flags.failureLevel, "Failed to " + lowerCase(operation));
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
        case Level::TRACE  : return TEXT_COLOR_GRAY        "[TRACE] ";
        case Level::DEBUG  : return TEXT_COLOR_WHITE       "[DEBUG] ";
        case Level::INFO   : return TEXT_COLOR_WHITE_BOLD  "[INFO]  ";
        case Level::WARNING: return TEXT_COLOR_ORANGE      "[WARNING] ";
        case Level::ERROR  : return TEXT_COLOR_FLAME       "[ERROR] ";
        case Level::FATAL  : return TEXT_COLOR_RED_BOLD    "[FATAL] ";

        default            : return "[UNKNOWN]";
    }
}

std::string Logger::lowerCase(const std::string& str) const{
    if (str.empty()) return str;

    std::string result = str;
    result[0] = static_cast<char>( std::tolower(static_cast<unsigned char>(result[0])));

    return result;
}
