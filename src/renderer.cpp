#include "glm/trigonometric.hpp"
#include <renderer.hpp>
#include <logger.hpp>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tol/tiny_obj_loader.h>



/*
std::vector<Vertex> vertices{
    // Cube
    {{-0.5f,  0.5f, -0.5f}, {0.8f , 0.15f, 0.6f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.15f, 0.2f , 0.9f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.95f, 0.4f , 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.5f , 0.8f , 0.1f}, {1.0f, 0.0f}},

    {{-0.5f,  0.5f,  0.5f}, {0.4f , 0.5f , 0.2f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.1f , 0.7f , 0.3f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.7f , 0.1f , 0.6f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.1f , 0.3f , 0.8f}, {1.0f, 0.0f}},

    // Square
    {{-1.0f, -1.3f,  1.0f}, {0.1f , 0.3f , 0.8f}, {1.0f, 1.0f}},
    {{ 1.0f, -1.3f,  1.0f}, {0.1f , 0.3f , 0.8f}, {0.0f, 1.0f}},
    {{ 1.0f, -0.8f, -0.5f}, {0.1f , 0.3f , 0.8f}, {0.0f, 0.0f}},
    {{-1.0f, -0.8f, -0.5f}, {0.1f , 0.3f , 0.8f}, {1.0f, 0.0f}},
};

const std::vector<uint16_t> vertexIndices{ 
    // Cube
    4, 7, 6, 
    6, 5, 4, 

    0, 1, 2, 
    2, 3, 0, 

    7, 3, 2, 
    2, 6, 7, 

    5, 1, 0, 
    0, 4, 5, 

    6, 2, 1, 
    1, 5, 6, 

    4, 0, 3, 
    3, 7, 4, 

    // Square
    8, 9, 10,
    10, 11, 8
};
*/


//==================================Main Functions==================================
void Renderer::init(GLFWwindow * appWindow){
    LOG_DEBUG("Initializing renderer");

    enableValidationLayers = Logger::get().getMinLevel() <= Logger::Level::ERROR;
    enableValidationLayers? LOG_DEBUG("Validation layers enabled") : LOG_DEBUG("Validation layers disabled");

    window = appWindow;
    
    createVulkanInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createSwapchainImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPools();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createGraphicsCommandBuffers();
    createSyncObjects();
}

void Renderer::drawFrame(){
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        LOG_TRACE("Swapchain out of date - recreating swapchain");
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_FATAL("Failed to acquire swapchain image");
    }

    // Only reset fence if work is getting submitted to avoid deadlocks
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    updateUniformBuffer(currentFrame);

    vkResetCommandBuffer(graphicsCommandBuffers[currentFrame], 0);
    recordCommandBuffer(graphicsCommandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]      = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphores;
    submitInfo.pWaitDstStageMask      = waitStages;
    submitInfo.commandBufferCount     = 1;
    submitInfo.pCommandBuffers        = &graphicsCommandBuffers[currentFrame];

    VkSemaphore signalSemaphores[]    = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount   = 1;
    submitInfo.pSignalSemaphores      = signalSemaphores;

    LOG_RESULT_SILENT(
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]),
        "Submit draw command buffer"
    );

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapchains[]    = { swapchain };
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        LOG_TRACE("Swapchain out of date (or suboptimal) - recreating swapchain");
        framebufferResized = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        LOG_FATAL("Failed to present swapchain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::deviceWait(){
    vkDeviceWaitIdle(device);
}

void Renderer::cleanup(){ 
    LOG_DEBUG("Renderer cleanup");

    LOG_TRACE("Cleanup : swapchain");
    cleanupSwapchain();

    LOG_TRACE("Cleanup : texture images");
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    LOG_TRACE("Cleanup : uniform buffers");
    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    LOG_TRACE("Cleanup : descriptor pool");
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    LOG_TRACE("Cleanup : pipeline");
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    LOG_TRACE("Cleanup : index buffer");
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    LOG_TRACE("Cleanup : vertex buffer");
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    LOG_TRACE("Cleanup : sync objects");
    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    LOG_TRACE("Cleanup : command pools");
    if (transferCommandPool == graphicsCommandPool) {
        vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
    } else {
        vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
        vkDestroyCommandPool(device, transferCommandPool, nullptr);
    }

    LOG_TRACE("Cleanup : device");
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        LOG_TRACE("Cleanup : validation debug messenger");
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    LOG_TRACE("Cleanup : surface");
    vkDestroySurfaceKHR(instance, surface, nullptr);

    LOG_TRACE("Cleanup : Vulkan instance");
    vkDestroyInstance(instance, nullptr);
}

void Renderer::createVulkanInstance(){
    VkApplicationInfo appInfo{};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion  = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pApplicationName    = "VulkanApp";
    appInfo.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName         = "No Engine";
    appInfo.apiVersion          = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;


    // Validation Layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers){
        if (checkValidationLayerSupport()){
            createInfo.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames     = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext                   = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } 
        else {
            LOG_WARNING("Validation layers enabled but not supported - disabling validation layers");
            enableValidationLayers = false;
        } 

    } else {
        createInfo.enabledLayerCount       = 0;
        createInfo.ppEnabledLayerNames     = nullptr;
        createInfo.pNext                   = nullptr;
    }


    // Extensions
    std::vector<const char*> extensions = getInstanceExtensions();

    if (!checkInstanceExtensionSupport(extensions)){
        LOG_FATAL("Vulkan instance extensions not supported");
    }

    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();


    LOG_RESULT(
        vkCreateInstance(&createInfo, nullptr, &instance),
        "Create Vulkan instance"
    );
}

void Renderer::createSurface(){
    LOG_RESULT(
        glfwCreateWindowSurface(instance, window, nullptr, &surface),
        "Create window surface"
    );
}

void Renderer::pickPhysicalDevice(){
    uint32_t deviceCount ;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (!deviceCount){
        LOG_FATAL("Failed to find GPU with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices){
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first < 0) {
        LOG_FATAL("Failed to find suitable GPU");
    } else {
        physicalDevice = candidates.rbegin()->second;
        LOG_RESULT(VK_SUCCESS, "Pick physical device");

        LOG_INFO("↓ Physical device picked ↓"); 
        LOG_DEVICE_INFO(physicalDevice);
    }
}

void Renderer::createLogicalDevice(){
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value(),
        indices.transferFamily.value()
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};

        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }


    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;


    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;


    // Extensions
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    
    LOG_RESULT(
        vkCreateDevice(physicalDevice, &createInfo, nullptr, &device),
        "Create logical device"
    );


    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value() , 0, &presentQueue);
    vkGetDeviceQueue(device, indices.transferFamily.value(), 0, &transferQueue);
}

void Renderer::createSwapchain(){
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D         extent        = chooseSwapExtent(swapchainSupport.capabilities);

    uint32_t           imageCount    = swapchainSupport.capabilities.minImageCount + 2;
    // 'maxImageCount == 0' is a special value to indicate that there is no maximum
    // Check if imageCount has exceeded the maximum
    if (swapchainSupport.capabilities.maxImageCount && imageCount > swapchainSupport.capabilities.maxImageCount) {
                       imageCount    = swapchainSupport.capabilities.maxImageCount; 
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface               = surface;
    createInfo.minImageCount         = imageCount;
    createInfo.imageFormat           = surfaceFormat.format;
    createInfo.imageColorSpace       = surfaceFormat.colorSpace;
    createInfo.imageExtent           = extent;
    createInfo.imageArrayLayers      = 1;
    createInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform          = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode           = presentMode;
    createInfo.clipped               = VK_TRUE;
    createInfo.oldSwapchain          = VK_NULL_HANDLE;

    LOG_RESULT(
        vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain),
        "Create swapchain"
    );

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    LOG_TRACE_S("Swapchain image count : " << imageCount);

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent      = extent;
}

void Renderer::createSwapchainImageViews(){
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        swapchainImageViews[i] = createImageView("swapchain", 
                                                 swapchainImages[i], 
                                                 swapchainImageFormat,
                                                 VK_IMAGE_ASPECT_COLOR_BIT
        );
    }
}

void Renderer::createRenderPass(){
    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapchainImageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment  = 0;
    colorAttachmentRef.layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    // Depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = findDepthFormat();
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment  = 1;
    depthAttachmentRef.layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass{}; 
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;


    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    
    std::array<VkAttachmentDescription, 2> attachments = {
        colorAttachment,
        depthAttachment
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;


    LOG_RESULT(
        vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass),
        "Create render pass"
    );
}

void Renderer::createDescriptorSetLayout(){
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = 0;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding            = 1;
    samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;


    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings    = bindings.data();

    LOG_RESULT(
        vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout),
        "Create descriptor set layout"
    );
}

void Renderer::createGraphicsPipeline(){
    // Shader Stages --------------------------------
    auto vertShaderCode = readFile(VERTEX_SHADER_CODE);
    auto fragShaderCode = readFile(FRAGMENT_SHADER_CODE);

    VkShaderModule vertShaderModule = createShaderModule("vertex", vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule("fragment", fragShaderCode);

    // - Vertex Shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    // - Fragment Shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };


    // Vertex Input --------------------------------
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();


    // Input Assembly --------------------------------
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;


    // Viewport & Scissor --------------------------------
    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount  = 1;


    // Rasterizer --------------------------------
    VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
    rasterizerInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable        = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizerInfo.lineWidth               = 1.0f;
    rasterizerInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizerInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerInfo.depthBiasEnable         = VK_FALSE;


    // Multisampling --------------------------------
    VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
    multisamplingInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable  = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


    // Depth Stencil --------------------------------
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable       = VK_TRUE;
    depthStencilInfo.depthWriteEnable      = VK_TRUE;
    depthStencilInfo.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable     = VK_FALSE;


    // Color Blending --------------------------------
    // - Attachment
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | 
                                          VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    // - Create Info
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable   = VK_FALSE;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments    = &colorBlendAttachment;


    // Dynamic States --------------------------------
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates    = dynamicStates.data();
    

    // Pipeline Layout --------------------------------
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts    = &descriptorSetLayout;

    LOG_RESULT(
        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout),
        "Create pipeline layout"
    );


    // Pipeline Creation --------------------------------
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState      = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState   = &multisamplingInfo;
    pipelineInfo.pDepthStencilState  = &depthStencilInfo;
    pipelineInfo.pColorBlendState    = &colorBlendInfo;
    pipelineInfo.pDynamicState       = &dynamicStateInfo;

    pipelineInfo.layout              = pipelineLayout;
    pipelineInfo.renderPass          = renderPass;
    pipelineInfo.subpass             = 0;

    LOG_RESULT(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline),
        "Create graphics pipeline"
    );


    // Cleanup --------------------------------
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Renderer::createFramebuffers(){
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i=0; i < swapchainImageViews.size(); ++i) {
        std::array<VkImageView, 2> attachments = {
            swapchainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass      = renderPass;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments    = attachments.data();
        createInfo.width           = swapchainExtent.width;
        createInfo.height          = swapchainExtent.height;
        createInfo.layers          = 1;

        LOG_RESULT(
            vkCreateFramebuffer(device, &createInfo, nullptr, &swapchainFramebuffers[i]),
            "Create framebuffer " + std::to_string(i)
        );
    }
}

void Renderer::createCommandPools(){
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo createInfo{};

    // Graphics command pool
    createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    LOG_RESULT(
        vkCreateCommandPool(device, &createInfo, nullptr, &graphicsCommandPool),
        "Create graphics command pool"
    );


    // Transfer command pool
    if (queueFamilyIndices.transferFamily.value() == queueFamilyIndices.graphicsFamily.value()) {
        LOG_TRACE("Using graphics command pool as transfer command pool - same family queue");
        transferCommandPool = graphicsCommandPool;
    }
    else {
        createInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
        createInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;       // Optimization for short-lived command buffers

        LOG_RESULT(
            vkCreateCommandPool(device, &createInfo, nullptr, &transferCommandPool),
            "Create transfer command pool"
        );
    }
}

void Renderer::createDepthResources(){
    VkFormat depthFormat = findDepthFormat();   

    createImage("depth", 
                swapchainExtent.width, swapchainExtent.height, 
                depthFormat, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                depthImage, depthImageMemory
    );
    depthImageView = createImageView("depth", 
                                     depthImage,
                                     depthFormat,
                                     VK_IMAGE_ASPECT_DEPTH_BIT
    );

    transitionImageLayout(depthImage, 
                          depthFormat, 
                          VK_IMAGE_LAYOUT_UNDEFINED, 
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );
}

void Renderer::createTextureImage(){
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(MODEL_TEXTURE, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) LOG_FATAL("Failed to load texture image");

    VkDeviceSize imageSize = texWidth * texHeight * 4;


    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer("staging",
                 imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory
    );


    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    
    createImage("texture", 
                texWidth, texHeight, 
                VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage, textureImageMemory
    );


    transitionImageLayout(textureImage, 
                          VK_FORMAT_R8G8B8A8_SRGB, 
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    copyBufferToImage(stagingBuffer, textureImage,
                      static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight)
    );

    transitionImageLayout(textureImage,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );


    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Renderer::createTextureImageView(){
    textureImageView = createImageView("texture", 
                                       textureImage, 
                                       VK_FORMAT_R8G8B8A8_SRGB,
                                       VK_IMAGE_ASPECT_COLOR_BIT
    );
}

void Renderer::createTextureSampler(){
    VkSamplerCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter        = VK_FILTER_LINEAR;
    createInfo.minFilter        = VK_FILTER_LINEAR;
    createInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    createInfo.maxAnisotropy    = deviceProperties.limits.maxSamplerAnisotropy;

    createInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable           = VK_FALSE;
    createInfo.compareOp               = VK_COMPARE_OP_ALWAYS;

    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod     = 0.0f;
    createInfo.maxLod     = 0.0f;

    LOG_RESULT(
        vkCreateSampler(device, &createInfo, nullptr, &textureSampler), 
        "Create texture sampler"
    );
}

void Renderer::loadModel(){
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL)) {
        LOG_ERROR_S("Failed to load model : " << warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            // vertices.push_back(vertex);
            // vertexIndices.push_back(vertexIndices.size());
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            vertexIndices.push_back(uniqueVertices[vertex]);
        }
    }
}

void Renderer::createVertexBuffer(){
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer("staging",
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer("vertex",
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vertexBuffer, vertexBufferMemory
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Renderer::createIndexBuffer(){
    VkDeviceSize bufferSize = sizeof(vertexIndices[0]) * vertexIndices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer("staging",
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertexIndices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer("index",
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 indexBuffer, indexBufferMemory
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Renderer::createUniformBuffers(){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        createBuffer("uniform", 
                     bufferSize, 
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                     uniformBuffers[i], uniformBuffersMemory[i]
        );

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void Renderer::createDescriptorPool(){
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes    = poolSizes.data();
    createInfo.maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    LOG_RESULT(
        vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool),
        "Create descriptor pool"
    );
}

void Renderer::createDescriptorSets(){
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts        = layouts.data();


    LOG_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()),
        "Allocate descriptor sets"
    );


    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range  = VK_WHOLE_SIZE;     // sizeof(UniformBufferObject)

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView   = textureImageView;
        imageInfo.sampler     = textureSampler;


        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = descriptorSets[i];
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo     = &bufferInfo;

        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = descriptorSets[i];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo      = &imageInfo;


        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::createGraphicsCommandBuffers(){
    graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = graphicsCommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(graphicsCommandBuffers.size());

    LOG_RESULT(
        vkAllocateCommandBuffers(device, &allocInfo, graphicsCommandBuffers.data()),
        "Allocate graphics command buffers"
    );
}

void Renderer::createSyncObjects(){
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        LOG_RESULT(
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]),
            "Create sync object: imageAvailableSemaphore " + std::to_string(i)
        );
        LOG_RESULT(
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]),
            "Create sync object: renderFinishedSemaphore " + std::to_string(i)
        );
        LOG_RESULT(
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]),
            "Create sync object: inFlightFence " + std::to_string(i)
        );
    }
}


void Renderer::recreateSwapchain(){
    // Handle minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }


    vkDeviceWaitIdle(device);

    cleanupSwapchain();

    createSwapchain();
    createSwapchainImageViews();
    createDepthResources();
    createFramebuffers();
}

void Renderer::cleanupSwapchain(){
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (size_t i=0; i < swapchainFramebuffers.size(); ++i) {
        vkDestroyFramebuffer(device, swapchainFramebuffers[i], nullptr);
    }

    for (size_t i=0; i < swapchainImageViews.size(); ++i) {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr); 
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}


//==================================Validation==================================
VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
){
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_TRACE_S("Validation : "   << pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:              // Currently disabled - see populateDebugMessengerCreateInfo()
            LOG_INFO_S("Validation : "    << pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARNING_S("Validation : " << pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR_S("Validation : "   << pCallbackData->pMessage);
            break;
        default: ;
    }

    return VK_FALSE;
}

VkResult Renderer::CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Renderer::DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Renderer::setupDebugMessenger(){
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    LOG_RESULT(
        CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger),
        "Set up debug messenger"
    );
}


//==================================Helper Functions==================================
std::vector<const char*> Renderer::getInstanceExtensions(){
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);    

    if (enableValidationLayers){
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Renderer::checkInstanceExtensionSupport(std::vector<const char*> &extensions){
    uint32_t supportedExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);

    std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

    for (const auto& extension : extensions) {
        bool supported = false;
        for (auto& supportedExtension : supportedExtensions) {
            if (!strcmp(supportedExtension.extensionName, extension)){
                supported = true;
                break;
            }
        }

        if (!supported) return false;
    }

    return true;
}

bool Renderer::checkValidationLayerSupport(){
    uint32_t supportedLayerCount = 0;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

    for (const auto& layer : validationLayers) {
        bool supported = false;
        for (const auto& supportedLayer : supportedLayers) {
            if (!strcmp(layer, supportedLayer.layerName)){
                supported = true;
                break;
            }
        }
        if (!supported) {
            return false;
        }
    }

    return true;
}

void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo){
    createInfo = {};
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType     = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData       = nullptr;
}

QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device){
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        // Dedicated transfer queue family
        else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) break;

        ++i;
    }

    // Set the transfer family as the graphics family if failed to find dedicated transfer queue family
    //  - Any queue with VK_QUEUE_GRAPHICS_BIT capabilities implicitly support VK_QUEUE_TRANSFER_BIT operations
    if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value() ) {
        indices.transferFamily = indices.graphicsFamily;
    } 

    return indices;
}

int Renderer::rateDeviceSuitability(VkPhysicalDevice device){
    int score = 0;

    QueueFamilyIndices indices = findQueueFamilies(device);

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    LOG_TRACE_S("Rating device suitability : " << deviceProperties.deviceName);

    // Must check for swapchain extension support before querying for details
    if (!checkDeviceExtensionSupport(device)) return -1;

    SwapchainSupportDetails swapchainDetails = querySwapchainSupport(device);


    bool required = indices.isComplete()                   &&
                    !swapchainDetails.formats.empty()      &&
                    !swapchainDetails.presentModes.empty() &&
                    deviceFeatures.samplerAnisotropy;

    if (!required) return -1;


    //  - Discrete GPU
    score += (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)? 1000 : 0;
    //  - Dedicated transfer queue family
    score += (indices.transferFamily.value() != indices.graphicsFamily.value())   ? 500  : 0;

    return score;
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device){
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapchainSupportDetails Renderer::querySwapchainSupport(VkPhysicalDevice device){
    SwapchainSupportDetails details;

    // Capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);


    // Formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }


    // Presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats){
    for (const auto& availableFormat: availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat; 
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availableModes){
    for (const auto& availableMode : availableModes) {
        if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availableMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities){
    // Special value to indicate that the extent should be chosen and set manually
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } 

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);    // In pixels

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );

    actualExtent.height = std::clamp(actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return actualExtent;
}

std::vector<char> Renderer::readFile(const std::string &fileName){
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        LOG_FATAL("Failed to open file");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VkShaderModule Renderer::createShaderModule(const std::string& name, const std::vector<char> &code){
    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo createInfo{};

    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    LOG_RESULT(
        vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
        "Create " + name + " shader module"
    );

    return shaderModule;
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex){
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    LOG_RESULT_SILENT(
        vkBeginCommandBuffer(commandBuffer, &beginInfo),
        "Begin recording command buffer"
    );


    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = swapchainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;


    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float_t>(swapchainExtent.width);
        viewport.height   = static_cast<float_t>(swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset    = {0, 0};
        scissor.extent    = swapchainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[]   = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vertexIndices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);


    LOG_RESULT_SILENT(
        vkEndCommandBuffer(commandBuffer),
        "End recording command buffer"
    );
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties){
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i=0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    LOG_FATAL("Failed to find suitable memory type");
    abort();      // Already called in LOG_FATAL();
}

void Renderer::createBuffer(const std::string& name,
                            VkDeviceSize size,
                            VkBufferUsageFlags usage,
                            VkMemoryPropertyFlags properties,
                            VkBuffer& buffer, VkDeviceMemory& bufferMemory
){
    VkBufferCreateInfo createInfo{};
    createInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size        = size;
    createInfo.usage       = usage;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.transferFamily.value()
    };

    if (indices.transferFamily.value() != indices.graphicsFamily.value()) {
        createInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }


    LOG_RESULT(
        vkCreateBuffer(device, &createInfo, nullptr, &buffer),
        "Create " + name + " buffer"
    );


    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    LOG_RESULT(
        vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory),
        "Allocate " + name + " buffer memory"
    );


    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Renderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
    LOG_TRACE("Copying buffer");

    VkCommandBuffer transferCommandBuffer = beginSingleTimeCommands(transferCommandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size      = size;

        vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(transferCommandBuffer, transferCommandPool, transferQueue);
}

void Renderer::updateUniformBuffer(uint32_t frame){
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f),
                            0.0f,
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view  = glm::lookAt(glm::vec3(2.0f, 2.0f + glm::sin(time), 1.0f + glm::cos(time)), 
                            glm::vec3(0.0f, 0.0f, 0.0f), 
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj  = glm::perspective(glm::radians(60.0f),
                                 swapchainExtent.width / (float) swapchainExtent.height,
                                 0.1f, 
                                 10.0f);

    // The Y axis is pointing down in Vulkan (glm was made for OpenGL - Y axis pointing up)
    // Must flip rasterizer front face so that backface culling works as intended
    ubo.proj[1][1] *= -1;


    memcpy(uniformBuffersMapped[frame], &ubo, sizeof(ubo));
}

void Renderer::createImage(const std::string& name, 
                           uint32_t width, uint32_t height, 
                           VkFormat format, VkImageTiling tiling, 
                           VkImageUsageFlags usage, 
                           VkMemoryPropertyFlags properties,
                           VkImage& image, VkDeviceMemory& imageMemory
){
    VkImageCreateInfo createInfo{};
    createInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType     = VK_IMAGE_TYPE_2D;
    createInfo.extent.width  = width;
    createInfo.extent.height = height;
    createInfo.extent.depth  = 1;
    createInfo.mipLevels     = 1;
    createInfo.arrayLayers   = 1;
    createInfo.format        = format;
    createInfo.tiling        = tiling;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage         = usage;
    createInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.transferFamily.value()
    };

    if (indices.transferFamily.value() != indices.graphicsFamily.value()) {
        createInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    LOG_RESULT(
        vkCreateImage(device, &createInfo, nullptr, &image), 
        "Create " + name + " image"
    );


    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    LOG_RESULT(
        vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory), 
        "Allocate " + name + " image memory"
    );

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkCommandBuffer Renderer::beginSingleTimeCommands(VkCommandPool &commandPool){
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer &commandBuffer, VkCommandPool &commandPool, VkQueue &queue){
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(graphicsCommandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = oldLayout;
        barrier.newLayout           = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = image;


        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        ){
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } 
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        ){
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } 
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                 newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } 
        else {
            LOG_ERROR("Unsupported layout transition");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer, graphicsCommandPool, graphicsQueue);
}

void Renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(transferCommandPool);

        VkBufferImageCopy region{};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer, transferCommandPool, transferQueue);
}

VkImageView Renderer::createImageView(const std::string& name, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags){
    VkImageView imageView;

    VkImageViewCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format   = format;

    createInfo.subresourceRange.aspectMask     = aspectFlags;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    LOG_RESULT(
        vkCreateImageView(device, &createInfo, nullptr, &imageView),
        "Create " + name + " image view"
    );

    return imageView;
}

VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features){
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && 
           (props.linearTilingFeatures & features) == features)
        {
            return format;
        } 
        else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    LOG_FATAL("Failed to find image format");
    abort();  // Already called in LOG_FATAL()
}

VkFormat Renderer::findDepthFormat(){
    return findSupportedFormat(
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        }, 
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool Renderer::hasStencilComponent(VkFormat format){
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}
