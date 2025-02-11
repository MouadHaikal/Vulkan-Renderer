#include "../include/VkRenderer.hpp"

#include <iostream>

int Renderer::init(GLFWwindow * newWindow){
    window = newWindow;
    try {
        createVulkanInstance();
        pickPhysicalDevice();
        createLogicalDevice();    
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Renderer::createVulkanInstance(){
    VkApplicationInfo appInfo = {};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion  = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pApplicationName    = "VulkanApp";
    appInfo.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName         = "VulkanEngine";
    appInfo.apiVersion          = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = 0;
    createInfo.ppEnabledExtensionNames = nullptr;
    createInfo.enabledLayerCount       = 0;
    createInfo.ppEnabledLayerNames     = nullptr;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS){
        throw std::runtime_error("Failed to create vulkan instanc!");
    }
}
