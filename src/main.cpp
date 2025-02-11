#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

#include "../include/VkRenderer.hpp"

GLFWwindow * window;
Renderer renderer;

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

void initWindow(std::string windowName, int width, int height){
    glfwInit();

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
}

int main(){
    // Initialization
    initWindow("VulkanApp", WIDTH, HEIGHT);

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

    return EXIT_SUCCESS;
}
