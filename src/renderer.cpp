#include "renderer.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <map>


//==================================Main Functions==================================
int Renderer::init(GLFWwindow * appWindow){
    window = appWindow;
    try {
        createVulkanInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Renderer::cleanUp(){
    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr); 
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void Renderer::createVulkanInstance(){
    VkApplicationInfo appInfo = {};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion  = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pApplicationName    = "VulkanApp";
    appInfo.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName         = "No Engine";
    appInfo.apiVersion          = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;

    // Extensions
    std::vector<const char*> extensions = getRequiredExtensions();

    if (!checkInstanceExtensionSupport(extensions)){
        throw std::runtime_error("Vulkan instance extensions not supported!");
    }

    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Validation Layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers){
        if (!checkValidationLayerSupport()){
            throw std::runtime_error("DEBUG: Validation layers not supported!");
        } 

        createInfo.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames     = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext                   = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

    } else {
        createInfo.enabledLayerCount       = 0;
        createInfo.ppEnabledLayerNames     = nullptr;
        createInfo.pNext                   = nullptr;
    }


    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS){
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void Renderer::createSurface(){
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void Renderer::pickPhysicalDevice(){
    uint32_t devciceCount ;
    vkEnumeratePhysicalDevices(instance, &devciceCount, nullptr);

    if (!devciceCount){
        throw std::runtime_error("Failed to find a GPU with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(devciceCount);
    vkEnumeratePhysicalDevices(instance, &devciceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices){
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("Failed to find suitable GPU!");
    }
}

void Renderer::createLogicalDevice(){
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

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


    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;

    // Extensions
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    // Validation Layers
    // -- Optional (device specific layers are deprecated)
    if (enableValidationLayers){
        createInfo.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames     = validationLayers.data();
    } else {
        createInfo.enabledLayerCount       = 0;
        createInfo.ppEnabledLayerNames     = nullptr;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void Renderer::createSwapchain(){
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D         extent        = chooseSwapExtent(swapchainSupport.capabilities);
    std::cout << extent.width << std::endl;

    uint32_t           imageCount    = swapchainSupport.capabilities.minImageCount + 1;
    if (!swapchainSupport.capabilities.maxImageCount && imageCount > swapchainSupport.capabilities.maxImageCount) {
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
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
    }

    createInfo.preTransform          = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode           = presentMode;
    createInfo.clipped               = VK_TRUE;
    createInfo.oldSwapchain          = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain!");
    }

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent      = extent;
}

void Renderer::createImageViews(){
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        VkImageViewCreateInfo createInfo{};

        createInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image        = swapchainImages[i];
        createInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format       = swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}


//==================================Validation==================================
VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
){
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

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

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS){
        throw std::runtime_error("Failed to set up debug messenger!");
    }
}


//==================================Helper Functions==================================
std::vector<const char*> Renderer::getRequiredExtensions(){
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
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
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

    int i=0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) break;

        ++i;
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

    SwapchainSupportDetails swapchainDetails = querySwapchainSupport(device);


    // Required
    bool required = indices.isComplete() &&
                    checkDeviceExtensionSupport(device) &&
                    !swapchainDetails.formats.empty() &&
                    !swapchainDetails.presentModes.empty();

    if (!required) return 0;


    // Secondary
    score += (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)? 1000 : 0;
    score += deviceProperties.limits.maxImageDimension2D;

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

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

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
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } 

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

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
