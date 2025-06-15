#include <logger.hpp>


Logger* Logger::instance = nullptr;

Logger& Logger::get(){
    if (!instance) instance = new Logger(); 
    return *instance;
}

void Logger::destroy(){
    if (!instance) return;

    delete instance;
    instance = nullptr;
}

void Logger::log(Level level, const std::string& message) const{
    if (level < minLevel) return;

    fmt::print(fmt::fg(toColor(level)), "{} {}\n", toCstr(level), message);

    if (level == Level::FATAL) {
        fmt::print(fmt::fg(toColor(Level::FATAL)), "Aborting\n");
        std::abort();    
    }
}

void Logger::logResult(VkResult result, const std::string& operation, const LogFlags& flags) const{
    if (result == VK_SUCCESS) {
        if (flags.traceSuccess) {
            log(Level::TRACE, operation);
        }

    } else{
        log(flags.failureLevel, "Failed to " + lowerCase(operation));
    }
}

void Logger::logDeviceInfo(VkPhysicalDevice device, QueueFamilyIndices queueFamilyIndices) const{
    if (minLevel > Level::INFO) return;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

    float memorySize = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
        if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            memorySize = memoryProperties .memoryHeaps[i].size / (1073741824.f);  // 1024 * 1024 * 1024
            break;
        }
    }

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

    LOG_INFO_S("- Memory : " << memorySize << " GB");

    LOG_INFO_S("- API version : " << 
               VK_API_VERSION_MAJOR(deviceProperties.apiVersion) << "." <<
               VK_API_VERSION_MINOR(deviceProperties.apiVersion) << "." <<
               VK_API_VERSION_PATCH(deviceProperties.apiVersion));

    LOG_DEBUG("- Queue family indices :");
        LOG_DEBUG_S("\tGraphics : " << queueFamilyIndices.graphicsFamily.value());
        LOG_DEBUG_S("\tPresent  : " << queueFamilyIndices.presentFamily.value());
        LOG_DEBUG_S("\tTransfer : " << queueFamilyIndices.transferFamily.value());

    LOG_DEBUG_S("- Max push constants size : " << deviceProperties.limits.maxPushConstantsSize << " bytes");

    LOG_INFO("=================================================");
}


const char* Logger::toCstr(Level level) const{
    switch (level) {
        case Level::TRACE  : return "[TRACE]";
        case Level::DEBUG  : return "[DEBUG]";
        case Level::INFO   : return "[INFO] ";
        case Level::WARN   : return "[WARN] ";
        case Level::ERROR  : return "[ERROR]";
        case Level::FATAL  : return "[FATAL]";
        default: return "";
    }
}

fmt::color Logger::toColor(Level level) const{
    switch (level) {
        case Level::TRACE  : return fmt::color::light_gray;
        case Level::DEBUG  : return fmt::color::white;
        case Level::INFO   : return fmt::color::light_sky_blue;
        case Level::WARN   : return fmt::color::orange;
        case Level::ERROR  : return fmt::color::orange_red;
        case Level::FATAL  : return fmt::color::crimson;
        default: return fmt::color::dark_orange;
    }
}

std::string Logger::lowerCase(const std::string& str) const{
    if (str.empty()) return str;

    std::string result = str;
    result[0] = static_cast<char>( std::tolower(static_cast<unsigned char>(result[0])));

    return result;
}
