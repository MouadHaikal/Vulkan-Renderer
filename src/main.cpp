#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdint>

#include "../include/Renderer.hpp"

GLFWwindow * window;
Renderer renderer;

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

void initWindow(const char* windowName){
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    float xScale, yScale;
    
    if (monitor){
        glfwGetMonitorContentScale(monitor, &xScale, &yScale);
    } else {
        std::cerr << "Failed to get primary monitor!" << std::endl;
        xScale = 1.0f;
        yScale = 1.0f;
    }

    window = glfwCreateWindow(WIDTH/xScale, HEIGHT/yScale, windowName, nullptr, nullptr);
}

int main(){
    // Initialization
    glfwInit();
    initWindow("VulkanApp");

    if (renderer.init(window) != EXIT_SUCCESS){
        return EXIT_FAILURE;
    }


    // Main loop
    while (glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }


    // Clean up
    glfwDestroyWindow(window);
    glfwTerminate();
}
