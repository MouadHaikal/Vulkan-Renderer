#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <cstdint>
#include <array>
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

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete(){
        return graphicsFamily.has_value() &&
               presentFamily.has_value() &&
               transferFamily.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription                  getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};


class Renderer {
public:
    bool framebufferResized = false;


    void init(GLFWwindow * appWindow);

    void drawFrame();

    void deviceWait();

    void cleanup();

private:
    GLFWwindow *                 window;  

    VkInstance                   instance;

    VkDebugUtilsMessengerEXT     debugMessenger;

    VkPhysicalDevice             physicalDevice = VK_NULL_HANDLE;
    VkDevice                     device         = VK_NULL_HANDLE;

    VkQueue                      graphicsQueue;
    VkQueue                      presentQueue;
    VkQueue                      transferQueue;

    VkSurfaceKHR                 surface;

    VkSwapchainKHR               swapchain;
    std::vector<VkImage>         swapchainImages;
    VkFormat                     swapchainImageFormat;
    VkExtent2D                   swapchainExtent;
    std::vector<VkImageView>     swapchainImageViews;

    VkRenderPass                 renderPass;
    VkPipelineLayout             pipelineLayout;
    VkPipeline                   graphicsPipeline;

    std::vector<VkFramebuffer>   swapchainFramebuffers;

    VkBuffer                     vertexBuffer;
    VkDeviceMemory               vertexBufferMemory;

    VkCommandPool                graphicsCommandPool;
    VkCommandPool                transferCommandPool;

    std::vector<VkCommandBuffer> graphicsCommandBuffers;

    std::vector<VkSemaphore>     imageAvailableSemaphores;
    std::vector<VkSemaphore>     renderFinishedSemaphores;
    std::vector<VkFence>         inFlightFences;

    uint32_t                     currentFrame = 0;


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
    void createCommandPools();
    void createVertexBuffer();
    void createGraphicsCommandBuffers();
    void createSyncObjects();

    void recreateSwapchain();
    void cleanupSwapchain();


    //==================================Validation==================================
    bool enableValidationLayers = false;

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

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createBuffer(const std::string& bufferName, VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};
