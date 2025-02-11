#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
public:
    int init(GLFWwindow * newWindow);

private:
    GLFWwindow * window;  

    VkInstance instance;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    // Main Functions
    void createVulkanInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
};
