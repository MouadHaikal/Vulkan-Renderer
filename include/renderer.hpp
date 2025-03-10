#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>
#include <optional>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete(){
        return graphicsFamily.has_value();
    }
};

class Renderer {
public:
    Renderer() : physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE) {};

    int init(GLFWwindow * appWindow);

    void cleanUp();

private:
    GLFWwindow *              window;  

    VkInstance                instance;

    VkDebugUtilsMessengerEXT  debugMessenger;

    VkPhysicalDevice          physicalDevice;
    VkDevice                  device;


    //==================================Main Functions==================================
    void createVulkanInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();


    //==================================Validation==================================
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );
    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance, 
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    );
    static void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator
    );
    void setupDebugMessenger();


    //==================================Helper Functions==================================
    std::vector<const char*> getRequiredExtensions();
    bool checkInstanceExtensionSupport(std::vector<const char*> &extensions);
    bool checkValidationLayerSupport();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
};
