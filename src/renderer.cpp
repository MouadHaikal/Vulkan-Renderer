#include "renderer.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

int Renderer::init(GLFWwindow * appWindow){
    window = appWindow;
    try {
        createVulkanInstance();
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Renderer::cleanUp(){
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
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount   = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    if (!checkInstanceExtensionSupport(createInfo.enabledExtensionCount, createInfo.ppEnabledExtensionNames)){
        throw std::runtime_error("Vulkan instance extensions not supported!");
    }

    // Validation Layers
    createInfo.enabledLayerCount       = 0;
    createInfo.ppEnabledLayerNames     = nullptr;


    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS){
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void Renderer::pickPhysicalDevice(){
    uint32_t physicalDevciceCount ;
    vkEnumeratePhysicalDevices(instance, &physicalDevciceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices = {};
    physicalDevices.resize(physicalDevciceCount);

    vkEnumeratePhysicalDevices(instance, &physicalDevciceCount, physicalDevices.data());
}


bool Renderer::checkInstanceExtensionSupport(const uint32_t extensionCount, const char* const* extensionNames){
    uint32_t supportedExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);

    std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

    for (size_t i=0; i < extensionCount; ++i) {
        bool supported = false;
        for (auto& supportedExtension : supportedExtensions) {
            if (!strcmp(supportedExtension.extensionName, extensionNames[i])){
                supported = true;
                break;
            }
        }

        if (!supported) return false;
    }

    return true;
}
