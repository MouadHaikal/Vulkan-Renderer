#pragma once


#include <scene.hpp>


#define VIKING_ROOM_MODEL         "../assets/models/viking_room.obj"
#define VIKING_ROOM_MODEL_TEXTURE "../assets/textures/viking_room.png"

#define ANTEFIX_MODEL         "../assets/models/antefix.obj"
#define ANTEFIX_MODEL_TEXTURE "../assets/textures/antefix.png"

#define TEXTURE "../assets/textures/texture.jpg"

#define VERTEX_SHADER_CODE   "../shaders/spirv/vert.spv"  
#define FRAGMENT_SHADER_CODE "../shaders/spirv/frag.spv"  

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

    void drawFrame();

    void cleanup();

private:
    GLFWwindow *                 window;  

    uint32_t                     currentFrame = 0;

    VkInstance                   instance;

    VkDebugUtilsMessengerEXT     debugMessenger;

    VkPhysicalDevice             physicalDevice = VK_NULL_HANDLE;
    VkDevice                     device         = VK_NULL_HANDLE;

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

    VkDescriptorSetLayout        descriptorSetLayout;
    VkPipelineLayout             pipelineLayout;
    VkPipeline                   graphicsPipeline;

    std::vector<VkFramebuffer>   swapchainFramebuffers;

    VkImage                      depthImage;
    VkDeviceMemory               depthImageMemory;
    VkImageView                  depthImageView;

    VkSampler                    textureSampler;

    Scene                        scene;

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

    void createCommandPools();

    void createScene();

    void createTextureSampler();

    void createDescriptorSetLayout();
    void createRenderPass();
    void createGraphicsPipeline();

    void createDepthResources();
    void createFramebuffers();

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
    SwapchainSupportDetails  querySwapchainSupport(VkPhysicalDevice device);
    QueueFamilyIndices       findQueueFamilies(VkPhysicalDevice device);
    static std::vector<char> readFile(const std::string &fileName);
    VkFormat                 findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat                 findDepthFormat();


    //---Check----------------------------------------------------------------------------
    bool checkInstanceExtensionSupport(std::vector<const char*> &extensions);
    bool checkValidationLayerSupport();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    int  rateDeviceSuitability(VkPhysicalDevice device);


    //---Choose---------------------------------------------------------------------------
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availableModes);
    VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);


    //---Create---------------------------------------------------------------------------
    VkShaderModule createShaderModule(const std::string& name, const std::vector<char> &code);


    //---Modify---------------------------------------------------------------------------
    void updateUniformBuffer(uint32_t frame);


    //---Commands-------------------------------------------------------------------------
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);


    //---Validation-----------------------------------------------------------------------
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
};
