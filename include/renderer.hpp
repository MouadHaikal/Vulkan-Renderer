#pragma once


#include <scene.hpp>
#include <camera.hpp>
#include <inputHandler.hpp>
#include <gui.hpp>


const uint32_t API_VERSION = VK_API_VERSION_1_3;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const int MAX_FRAMES_IN_FLIGHT = 3;


class Renderer {
public:
    bool framebufferResized = false;


    void init(GLFWwindow * appWindow);

    void processInput(InputData input, float deltaTime);

    void drawFrame();

    void cleanup();

private:
    GLFWwindow *                 window;  

    uint32_t                     currentFrame = 0;

    VkInstance                   instance;

    VkDebugUtilsMessengerEXT     debugMessenger;

    VkPhysicalDevice             physicalDevice;
    VkDevice                     device;

    QueueFamilyIndices           queueFamilyIndices;

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

    VkDescriptorSetLayout        uboDescriptorSetLayout;
    VkDescriptorSetLayout        texDescriptorSetLayout;

    VkPipelineLayout             pipelineLayout;
    VkPipeline                   graphicsPipeline;

    std::vector<VkFramebuffer>   swapchainFramebuffers;

    VkImage                      colorImage;
    VkDeviceMemory               colorImageMemory;
    VkImageView                  colorImageView;

    VkImage                      depthImage;
    VkDeviceMemory               depthImageMemory;
    VkImageView                  depthImageView;

    std::shared_ptr<Scene>       scene;
    std::shared_ptr<Camera>      camera;

    // View & Projection matrices 
    std::vector<VkBuffer>        vpUniformBuffers;
    std::vector<VkDeviceMemory>  vpUniformBuffersMemory;
    std::vector<void*>           vpUniformBuffersMapped;

    // Point lights
    std::vector<VkBuffer>        plUniformBuffers;
    std::vector<VkDeviceMemory>  plUniformBuffersMemory;
    std::vector<void*>           plUniformBuffersMapped;

    // Directional lights
    std::vector<VkBuffer>        dlUniformBuffers;
    std::vector<VkDeviceMemory>  dlUniformBuffersMemory;
    std::vector<void*>           dlUniformBuffersMapped;

    VkDescriptorPool             uboDescriptorPool;
    VkDescriptorPool             texDescriptorPool;
    VkDescriptorPool             guiDescriptorPool;

    std::vector<VkDescriptorSet> uboDescriptorSets;
    VkDescriptorSet              texDescriptorSet;

    VkCommandPool                graphicsCommandPool;
    VkCommandPool                transferCommandPool;

    std::vector<VkCommandBuffer> graphicsCommandBuffers;

    std::vector<VkSemaphore>     imageAvailableSemaphores;
    std::vector<VkSemaphore>     renderFinishedSemaphores;
    std::vector<VkFence>         inFlightFences;

    Gui                          gui;

    std::shared_ptr<VkSampleCountFlagBits> msaaSamples = std::make_shared<VkSampleCountFlagBits>(VK_SAMPLE_COUNT_1_BIT);
    std::shared_ptr<float>       minSampleShading = std::make_shared<float>(.4f);



    //==================================Main Functions==================================
    void createVulkanInstance();

    void createSurface();

    void pickPhysicalDevice();
    void createLogicalDevice();

    void createSwapchain();
    void createSwapchainImageViews();

    void createCommandPools();

    void buildScene();

    void createDescriptorSetLayouts();
    void createRenderPass();
    void createGraphicsPipeline();

    void createColorResources();
    void createDepthResources();

    void createFramebuffers();

    void createUniformBuffers();

    void createDescriptorPools();
    void createDescriptorSets();

    void createGraphicsCommandBuffers();
    void createSyncObjects();

    void initGUI();


    void recreateRenderPass();

    void recreateSwapchain();
    void cleanupSwapchain();

    void recreateGraphicsPipeline();
    void recreateUniformBuffers();



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
    SwapchainSupportDetails  querySwapchainSupport(VkPhysicalDevice device);
    QueueFamilyIndices       findQueueFamilies(VkPhysicalDevice device);
    static std::vector<char> readFile(const std::string &fileName);
    VkFormat                 findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat                 findDepthFormat();


    //---Check----------------------------------------------------------------------------
    bool    checkInstanceExtensionSupport(std::vector<const char*> &extensions);
    bool    checkValidationLayerSupport();
    bool    checkDeviceExtensionSupport(VkPhysicalDevice device);
    int     rateDeviceSuitability(VkPhysicalDevice device);


    //---Choose---------------------------------------------------------------------------
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);


    //---Create---------------------------------------------------------------------------
    VkShaderModule createShaderModule(const std::string& name, const std::vector<char> &code);


    //---Modify---------------------------------------------------------------------------
    void updateUniformBuffers(uint32_t frame);


    //---Commands-------------------------------------------------------------------------
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);


    //---Validation-----------------------------------------------------------------------
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
};
