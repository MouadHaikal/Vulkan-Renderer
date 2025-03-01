#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
public:
    int init(GLFWwindow * newWindow);

private:
    GLFWwindow * window;  

    VkInstance instance;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;


    // Main Functions
    void createVulkanInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
};
