#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>

class Renderer {
public:
    Renderer() : physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE) {};

    int init(GLFWwindow * appWindow);

    void cleanUp();

private:
    GLFWwindow *          window;  

    VkInstance            instance;

    VkPhysicalDevice      physicalDevice;
    VkDevice              device;


    // Main Functions
    void createVulkanInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();


    // Helper Functions
    bool checkInstanceExtensionSupport(const uint32_t extensionCount, const char* const* extensionNames);
};
