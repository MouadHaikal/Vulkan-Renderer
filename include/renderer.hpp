#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <utilities.hpp>

#include <bits/stdc++.h>


#define TEXTURE "../assets/textures/texture.jpg"

#define VERTEX_SHADER_CODE   "../shaders/spirv/vert.spv"  
#define FRAGMENT_SHADER_CODE "../shaders/spirv/frag.spv"  

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const int MAX_FRAMES_IN_FLIGHT = 2;


class Renderer {
public:
    bool framebufferResized = false;


    void init(GLFWwindow * appWindow);

    void drawFrame();

    void deviceWait();

    void cleanup();

private:
    GLFWwindow *                 window;  

    uint32_t                     currentFrame = 0;

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

    VkDescriptorSetLayout        descriptorSetLayout;
    VkPipelineLayout             pipelineLayout;
    VkPipeline                   graphicsPipeline;

    std::vector<VkFramebuffer>   swapchainFramebuffers;

    VkImage                      depthImage;
    VkDeviceMemory               depthImageMemory;
    VkImageView                  depthImageView;

    VkImage                      textureImage;
    VkDeviceMemory               textureImageMemory;
    VkImageView                  textureImageView;
    VkSampler                    textureSampler;

    VkBuffer                     vertexBuffer;
    VkDeviceMemory               vertexBufferMemory;

    VkBuffer                     indexBuffer;
    VkDeviceMemory               indexBufferMemory;

    std::vector<VkBuffer>        uniformBuffers;
    std::vector<VkDeviceMemory>  uniformBuffersMemory;
    std::vector<void*>           uniformBuffersMapped;

    VkDescriptorPool             descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkCommandPool                graphicsCommandPool;
    VkCommandPool                transferCommandPool;

    std::vector<VkCommandBuffer> graphicsCommandBuffers;

    std::vector<VkSemaphore>     imageAvailableSemaphores;
    std::vector<VkSemaphore>     renderFinishedSemaphores;
    std::vector<VkFence>         inFlightFences;



    //==================================Main Functions==================================
    void createVulkanInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createSwapchainImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPools();
    void createDepthResources();
    void createFramebuffers();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
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

    //---Query----------------------------------------------------------------------------
    std::vector<const char*> getInstanceExtensions();
    QueueFamilyIndices       findQueueFamilies(VkPhysicalDevice device);
    SwapchainSupportDetails  querySwapchainSupport(VkPhysicalDevice device);
    static std::vector<char> readFile(const std::string &fileName);
    uint32_t                 findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat                 findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat                 findDepthFormat();


    //---Check----------------------------------------------------------------------------
    bool checkInstanceExtensionSupport(std::vector<const char*> &extensions);
    bool checkValidationLayerSupport();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    int  rateDeviceSuitability(VkPhysicalDevice device);
    bool hasStencilComponent(VkFormat format);


    //---Choose---------------------------------------------------------------------------
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availableModes);
    VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);


    //---Create---------------------------------------------------------------------------
    VkShaderModule createShaderModule(const std::string& name, const std::vector<char> &code);
    void           createBuffer(const std::string& name, 
                                VkDeviceSize size, 
                                VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags properties, 
                                VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void           createImage(const std::string& name, 
                               uint32_t width, uint32_t height, 
                               VkFormat format, VkImageTiling tiling, 
                               VkImageUsageFlags usage, 
                               VkMemoryPropertyFlags properties, 
                               VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView    createImageView(const std::string& name, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);


    //---Modify---------------------------------------------------------------------------
    void updateUniformBuffer(uint32_t frame);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);


    //---Copy-----------------------------------------------------------------------------
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


    //---Commands-------------------------------------------------------------------------
    void            recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool &commandPool);
    void            endSingleTimeCommands(VkCommandBuffer &commandBuffer, VkCommandPool &commandPool, VkQueue &queue);


    //---Validation-----------------------------------------------------------------------
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
};
