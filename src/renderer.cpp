#include <renderer.hpp>

#include <logger.hpp>



//==================================Main Functions==================================
void Renderer::init(GLFWwindow * appWindow){
    LOG_DEBUG("Initializing renderer");

    enableValidationLayers = Logger::get().getMinLevel() <= Logger::Level::ERROR;
    enableValidationLayers? LOG_DEBUG("Validation layers enabled") : LOG_DEBUG("Validation layers disabled");

    window = appWindow;
    
    createVulkanInstance();
    setupDebugMessenger();
    LOG_TRACE(SEP);

    createSurface();
    LOG_TRACE(SEP);

    pickPhysicalDevice();
    createLogicalDevice();
    LOG_TRACE(SEP);

    createSwapchain();
    createSwapchainImageViews();
    LOG_TRACE(SEP);

    createCommandPools();
    LOG_TRACE(SEP);

    createScene();
    LOG_TRACE(SEP);

    createDescriptorSetLayouts();
    createRenderPass();
    createGraphicsPipeline();
    LOG_TRACE(SEP);

    createDepthResources();
    createColorResources();
    LOG_TRACE(SEP);

    createFramebuffers();
    LOG_TRACE(SEP);

    createUniformBuffers();
    LOG_TRACE(SEP);

    createDescriptorPools();
    createDescriptorSets();
    LOG_TRACE(SEP);

    createGraphicsCommandBuffers();
    LOG_TRACE(SEP);

    createSyncObjects();
    LOG_TRACE(SEP);
}

void Renderer::processInput(InputData input, float deltaTime){
    camera.move(input.movement, deltaTime);
    camera.rotate(input.mouseOffset.x, input.mouseOffset.y);
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

    // Command buffer is implicitly reset at vkBeginCommandBuffer()
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

void Renderer::cleanup(){ 
    vkDeviceWaitIdle(device);

    LOG_DEBUG("Renderer cleanup");

    LOG_TRACE("Cleanup : scene");
    scene.cleanup();

    LOG_TRACE("Cleanup : swapchain");
    cleanupSwapchain();

    LOG_TRACE("Cleanup : uniform buffers");
    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    LOG_TRACE("Cleanup : descriptor pools");
    vkDestroyDescriptorPool(device, uboDescriptorPool, nullptr);
    vkDestroyDescriptorPool(device, texDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, uboDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, texDescriptorSetLayout, nullptr);

    LOG_TRACE("Cleanup : pipeline");
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    LOG_TRACE("Cleanup : sync objects");
    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    LOG_TRACE("Cleanup : command pools");
    if (transferCommandPool != graphicsCommandPool) vkDestroyCommandPool(device, transferCommandPool, nullptr);
    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);

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
    appInfo.pApplicationName    = "Vulkan Renderer";
    appInfo.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName         = "Custom Engine";
    appInfo.apiVersion          = VK_API_VERSION_1_3;

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

    std::multimap<ssize_t, VkPhysicalDevice> candidates;

    for (const auto& device : devices){
        ssize_t score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }


    if (candidates.rbegin()->first == -1) {
        LOG_FATAL("Failed to find suitable GPU");
        return;
    } 

    physicalDevice = candidates.rbegin()->second;

    LOG_RESULT(VK_SUCCESS, "Pick physical device");
    queueFamilyIndices = findQueueFamilies(physicalDevice);

    LOG_INFO("↓ Physical device picked ↓"); 
    LOG_DEVICE_INFO(physicalDevice, queueFamilyIndices);

    msaaSamples = getMaxUsableSampleCount();

    switch (msaaSamples) {
        case VK_SAMPLE_COUNT_64_BIT: LOG_INFO("MSAA : 64x");      break;
        case VK_SAMPLE_COUNT_32_BIT: LOG_INFO("MSAA : 32x");      break;
        case VK_SAMPLE_COUNT_16_BIT: LOG_INFO("MSAA : 16x");      break;
        case VK_SAMPLE_COUNT_8_BIT : LOG_INFO("MSAA : 8x");       break;
        case VK_SAMPLE_COUNT_4_BIT : LOG_INFO("MSAA : 4x");       break;
        case VK_SAMPLE_COUNT_2_BIT : LOG_INFO("MSAA : 2x");       break;
        case VK_SAMPLE_COUNT_1_BIT : LOG_INFO("MSAA : Disabled"); break;
        default:;
    }
}

void Renderer::createLogicalDevice(){
    QueueFamilyIndices indices = queueFamilyIndices;

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
    deviceFeatures.sampleRateShading = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.runtimeDescriptorArray                    = VK_TRUE;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;


    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext                   = &vulkan12Features;
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

    QueueFamilyIndices indices = queueFamilyIndices;
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
        utils::createImageView("swapchain", 
                               swapchainImages[i], 
                               swapchainImageFormat,
                               1, VK_IMAGE_ASPECT_COLOR_BIT,
                               swapchainImageViews[i], device
        );
    }
}

void Renderer::createCommandPools(){
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

void Renderer::createScene(){
    VulkanContext context;
    context.physicalDevice      = physicalDevice;
    context.device              = device;
    context.queueFamilyIndices  = queueFamilyIndices;
    context.graphicsQueue       = graphicsQueue;
    context.graphicsCommandPool = graphicsCommandPool;
    context.transferQueue       = transferQueue;
    context.transferCommandPool = transferCommandPool;

    Scene::setContext(context);

    scene.textureManager.addTexture("default.jpg");


    RawMesh planeMeshData = {
        // Vertices
        {
            {{10.f, 10.f, 0.f}, {1.f, 1.f}},
            {{10.f,-10.f, 0.f}, {1.f, 0.f}},
            {{-10.f,-10.f, 0.f}, {0.f, 0.f}},
            {{-10.f, 10.f, 0.f}, {0.f, 1.f}},
        },

        // Indices
        {
            0, 3, 2, 2, 1, 0
        },

        // Material
        {
            -1,
            glm::vec3(.02f, .02f, .02f)
        }
    };

    auto planeModel = scene.addModel(&planeMeshData);
    planeModel->modelMatrix = glm::translate(planeModel->modelMatrix, glm::vec3(0.f, 0.f, -.5f));


    auto antefixModel = scene.addModel("antefix.obj");
    scene.textureManager.addTexture("antefix.png");
    antefixModel->setUniformMaterial({scene.textureManager.getTextureIndex("antefix.png")});

    antefixModel->modelMatrix = glm::translate(antefixModel->modelMatrix, glm::vec3(6.f, .0f, -.5f));
    antefixModel->modelMatrix = glm::scale(antefixModel->modelMatrix, glm::vec3(20.f));
    antefixModel->modelMatrix = glm::rotate(antefixModel->modelMatrix, glm::radians(-30.f),glm::vec3(0.f, 0.f, 1.f));


    auto vikingRoomModel = scene.addModel("viking_room.obj");
    scene.textureManager.addTexture("viking_room.png");
    vikingRoomModel->setUniformMaterial({scene.textureManager.getTextureIndex("viking_room.png")});

    vikingRoomModel->modelMatrix = glm::translate(vikingRoomModel->modelMatrix, glm::vec3(-6.f, .0f, -.2f));
    vikingRoomModel->modelMatrix = glm::scale(vikingRoomModel->modelMatrix, glm::vec3(3.f));
    vikingRoomModel->modelMatrix = glm::rotate(vikingRoomModel->modelMatrix, glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));


    auto frankModel = scene.addModel("Frank.blend");

    frankModel->modelMatrix = glm::scale(frankModel->modelMatrix, glm::vec3(.1f));
    frankModel->modelMatrix = glm::translate(frankModel->modelMatrix, glm::vec3(0.f, 0.f, 7.f));
    frankModel->modelMatrix = glm::rotate(frankModel->modelMatrix, glm::radians(-133.f), glm::vec3(1.f, 0.f, 0.f));
}

void Renderer::createDescriptorSetLayouts(){
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = 0;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 1;
    createInfo.pBindings    = &uboLayoutBinding;

    LOG_RESULT(
        vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &uboDescriptorSetLayout),
        "Create UBO descriptor set layout"
    );



    VkDescriptorSetLayoutBinding texturesLayoutBinding{};   // Combined image samplers
    texturesLayoutBinding.binding         = 0;
    texturesLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturesLayoutBinding.descriptorCount = static_cast<uint32_t>(scene.textureManager.getTextureCount());
    texturesLayoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    createInfo.bindingCount = 1,
    createInfo.pBindings    = &texturesLayoutBinding;

    LOG_RESULT(
        vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &texDescriptorSetLayout), 
        "Create textures descriptor set layout"
    );
}

void Renderer::createRenderPass(){
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapchainImageFormat;
    colorAttachment.samples        = msaaSamples;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = msaaSamples > VK_SAMPLE_COUNT_1_BIT ? 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = findDepthFormat();
    depthAttachment.samples        = msaaSamples;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format         = swapchainImageFormat;
    colorAttachmentResolve.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    std::vector<VkAttachmentDescription> attachments; 
    if (msaaSamples > VK_SAMPLE_COUNT_1_BIT) {
        attachments = {
            colorAttachment,
            depthAttachment,
            colorAttachmentResolve
        };
    } 
    else {
        attachments = {
            colorAttachment,
            depthAttachment
        };
    }


    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment  = 0;
    colorAttachmentRef.layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment  = 1;
    depthAttachmentRef.layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass{}; 
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments     = msaaSamples > VK_SAMPLE_COUNT_1_BIT ? &colorAttachmentResolveRef : nullptr;

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

void Renderer::createGraphicsPipeline(){
    // Shader Stages --------------------------------
    auto vertShaderCode = readFile((dirShaders/"vert.spv").string());
    auto fragShaderCode = readFile((dirShaders/"frag.spv").string());

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
    multisamplingInfo.rasterizationSamples = msaaSamples;
    multisamplingInfo.sampleShadingEnable  = msaaSamples > VK_SAMPLE_COUNT_1_BIT ? VK_TRUE : VK_FALSE;
    multisamplingInfo.minSampleShading     = minSampleShading;


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
    VkPushConstantRange vertexPushConstantRange{};
    vertexPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vertexPushConstantRange.offset     = 0;
    vertexPushConstantRange.size       = sizeof(VertexPushConstant);

    VkPushConstantRange fragmentPushConstantRange{};
    fragmentPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentPushConstantRange.offset     = sizeof(VertexPushConstant);
    fragmentPushConstantRange.size       = sizeof(FragmentPushConstant);

    std::array<VkPushConstantRange, 2> pushConstantRanges = { vertexPushConstantRange, fragmentPushConstantRange }; 


    std::array<VkDescriptorSetLayout, 2> desciptorSetLayouts = { uboDescriptorSetLayout, texDescriptorSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = static_cast<uint32_t>(desciptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts            = desciptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges    = pushConstantRanges.data();

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

void Renderer::createColorResources(){
    if (msaaSamples == VK_SAMPLE_COUNT_1_BIT) return;

    VkFormat colorFormat = swapchainImageFormat;

    utils::createImage("color resource",
                       swapchainExtent.width, swapchainExtent.height, 
                       1, msaaSamples, 
                       colorFormat, VK_IMAGE_TILING_OPTIMAL, 
                       VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                       colorImage, colorImageMemory, 
                       physicalDevice, device, queueFamilyIndices
    );

    utils::createImageView("color resource", 
                           colorImage, colorFormat, 
                           1, VK_IMAGE_ASPECT_COLOR_BIT, 
                           colorImageView, device
    );
}

void Renderer::createDepthResources(){
    VkFormat depthFormat = findDepthFormat();   

    utils::createImage("depth", 
                       swapchainExtent.width, swapchainExtent.height,
                       1, msaaSamples,
                       depthFormat, VK_IMAGE_TILING_OPTIMAL, 
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                       depthImage, depthImageMemory,
                       physicalDevice, device, queueFamilyIndices
    );

    utils::createImageView("depth", 
                           depthImage,
                           depthFormat,
                           1, VK_IMAGE_ASPECT_DEPTH_BIT,
                           depthImageView, device
    );

    utils::transitionImageLayout(depthImage, 
                                 depthFormat, 1,
                                 VK_IMAGE_LAYOUT_UNDEFINED, 
                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                 device, graphicsQueue, graphicsCommandPool
    );
}

void Renderer::createFramebuffers(){
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i=0; i < swapchainImageViews.size(); ++i) {
        std::vector<VkImageView> attachments;
        if (msaaSamples > VK_SAMPLE_COUNT_1_BIT) {
            attachments = {
                colorImageView,
                depthImageView,
                swapchainImageViews[i]
            };
        } else {
            attachments = {
                swapchainImageViews[i],
                depthImageView
            };
        }

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

void Renderer::createUniformBuffers(){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        utils::createBuffer("uniform", 
                            bufferSize, 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            uniformBuffers[i], uniformBuffersMemory[i],
                            physicalDevice, device, queueFamilyIndices
        );

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void Renderer::createDescriptorPools(){
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes    = &poolSize;
    createInfo.maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    LOG_RESULT(
        vkCreateDescriptorPool(device, &createInfo, nullptr, &uboDescriptorPool),
        "Create UBO descriptor pool"
    );



    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = static_cast<uint32_t>(scene.textureManager.getTextureCount());

    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes    = &poolSize;
    createInfo.maxSets       = 1;

    LOG_RESULT(
        vkCreateDescriptorPool(device, &createInfo, nullptr, &texDescriptorPool),
        "Create textures descriptor pool"
    );
}

void Renderer::createDescriptorSets(){
    uboDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    
    std::vector<VkDescriptorSetLayout> uboLayouts(MAX_FRAMES_IN_FLIGHT, uboDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = uboDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts        = uboLayouts.data();


    LOG_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, uboDescriptorSets.data()),
        "Allocate ubo descriptor sets"
    );


    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObject);     // VK_WHOLE_SIZE


        VkWriteDescriptorSet descriptorWrite{};

        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = uboDescriptorSets[i];
        descriptorWrite.dstBinding      = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo     = &bufferInfo;


        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }



    allocInfo.descriptorPool     = texDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &texDescriptorSetLayout;

    LOG_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, &texDescriptorSet),
        "Allocate textures descriptor sets"
    );


    std::vector<VkDescriptorImageInfo> texturesInfo(scene.textureManager.getTextureCount());
    for (size_t j=0; j < scene.textureManager.getTextureCount(); ++j) {
        texturesInfo[j].sampler     = scene.textureManager.getTextureSampler(j);
        texturesInfo[j].imageView   = scene.textureManager.getTextureImageView(j);
        texturesInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }


    VkWriteDescriptorSet descriptorWrite{};

    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = texDescriptorSet;
    descriptorWrite.dstBinding      = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = static_cast<uint32_t>(texturesInfo.size());
    descriptorWrite.pImageInfo      = texturesInfo.data();

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
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

    createColorResources();
    createDepthResources();

    createFramebuffers();
    LOG_TRACE("-------------------------------------------------");
}

void Renderer::cleanupSwapchain(){
    if (msaaSamples > VK_SAMPLE_COUNT_1_BIT) {
        vkDestroyImageView(device, colorImageView, nullptr);
        vkDestroyImage(device, colorImage, nullptr);
        vkFreeMemory(device, colorImageMemory, nullptr);
    }

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

ssize_t Renderer::rateDeviceSuitability(VkPhysicalDevice device){
    ssize_t score = 0;

    QueueFamilyIndices indices = findQueueFamilies(device);

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);


    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext  = &vulkan12Features;

    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);


    LOG_TRACE_S("Rating device suitability : " << deviceProperties.deviceName);

    // Must check for swapchain extension support before querying for details
    if (!checkDeviceExtensionSupport(device)) return -1;

    SwapchainSupportDetails swapchainDetails = querySwapchainSupport(device);


    bool required = indices.isComplete()                    &&
                    !swapchainDetails.formats.empty()       &&
                    !swapchainDetails.presentModes.empty()  &&
                    deviceFeatures.samplerAnisotropy        &&
                    deviceFeatures.sampleRateShading        &&
                    vulkan12Features.runtimeDescriptorArray &&
                    vulkan12Features.shaderSampledImageArrayNonUniformIndexing;

    if (!required) return -1;


    //  - Discrete GPU
    score += (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)? 1000 : 0;
    //  - Dedicated transfer queue family
    score += (indices.transferFamily.value() != indices.graphicsFamily.value())   ? 500  : 0;
    //  - Max sampler anisotropy
    score += deviceProperties.limits.maxSamplerAnisotropy * 10;

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
        LOG_FATAL("Failed to open file (" + fileName + ")");
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

        std::array<VkDescriptorSet, 2> descriptorSets = { uboDescriptorSets[currentFrame], texDescriptorSet };
        vkCmdBindDescriptorSets(commandBuffer, 
                                VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 
                                static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(),
                                0, nullptr
        );

        scene.draw(commandBuffer, pipelineLayout);

    vkCmdEndRenderPass(commandBuffer);


    LOG_RESULT_SILENT(
        vkEndCommandBuffer(commandBuffer),
        "End recording command buffer"
    );
}

void Renderer::updateUniformBuffer(uint32_t frame){
    UniformBufferObject ubo{
        camera.getViewMatrix(),
        camera.getProjMatrix(swapchainExtent.width / (float) swapchainExtent.height)
    };

    // The Y axis is pointing down in Vulkan (glm was made for OpenGL - Y axis pointing up)
    // Must flip rasterizer front face so that backface culling works as intended
    ubo.proj[1][1] *= -1;


    memcpy(uniformBuffersMapped[frame], &ubo, sizeof(ubo));
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

    LOG_FATAL("Failed to find supported image format");
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

VkSampleCountFlagBits Renderer::getMaxUsableSampleCount(){
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    VkSampleCountFlags count = deviceProperties.limits.framebufferColorSampleCounts & 
                               deviceProperties.limits.framebufferDepthSampleCounts;

    if (count & VK_SAMPLE_COUNT_64_BIT) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (count & VK_SAMPLE_COUNT_32_BIT) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (count & VK_SAMPLE_COUNT_16_BIT) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (count & VK_SAMPLE_COUNT_8_BIT) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (count & VK_SAMPLE_COUNT_4_BIT) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (count & VK_SAMPLE_COUNT_2_BIT) {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}
