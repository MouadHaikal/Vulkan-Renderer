#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
public:
    int init(GLFWwindow * appWindow, const char* applicationName, const char* engineName);

private:
    GLFWwindow * window;  

    VkInstance instance;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device                 = VK_NULL_HANDLE;


    // Main Functions
    void createVulkanInstance(const char* applicationName, const char* engineName);
    void pickPhysicalDevice();
    void createLogicalDevice();
};
