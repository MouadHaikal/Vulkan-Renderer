#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <cstdint>
#include <vector>
#include <optional>


#define VERTEX_SHADER_CODE   "../shaders/spirv/vert.spv"  
#define FRAGMENT_SHADER_CODE "../shaders/spirv/frag.spv"  

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete(){
        return graphicsFamily.has_value() &&
               presentFamily.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Renderer {
public:
    Renderer() : physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE) {};

    int init(GLFWwindow * appWindow);

    void cleanup();

private:
    GLFWwindow *               window;  

    VkInstance                 instance;

    VkDebugUtilsMessengerEXT   debugMessenger;

    VkPhysicalDevice           physicalDevice;
    VkDevice                   device;

    VkQueue                    graphicsQueue;
    VkQueue                    presentQueue;

    VkSurfaceKHR               surface;

    VkSwapchainKHR             swapchain;
    std::vector<VkImage>       swapchainImages;
    VkFormat                   swapchainImageFormat;
    VkExtent2D                 swapchainExtent;
    std::vector<VkImageView>   swapchainImageViews;

    VkRenderPass               renderPass;
    VkPipelineLayout           pipelineLayout;
    VkPipeline                 graphicsPipeline;

    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkCommandPool              commandPool;


    //==================================Main Functions==================================
    void createVulkanInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();


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

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availableModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    static std::vector<char> readFile(const std::string &fileName);

    VkShaderModule createShaderModule(const std::vector<char> &code);
};
