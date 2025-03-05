#include "app.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

// Main Functions
void App::run(){
    init();
    mainLoop();
    cleanUp();
}

void App::init(){
    glfwInit();
    initWindow("VulkanApp");

    if (renderer.init(window) != EXIT_SUCCESS) {
        throw std::runtime_error("Failed to initialize renderer!");
    }
}

void App::mainLoop(){
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void App::cleanUp(){
    renderer.cleanUp();

    glfwDestroyWindow(window);
    glfwTerminate();
}


// Helper Functions
void App::initWindow(const char* title){
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

    window = glfwCreateWindow(WIDTH/xScale, HEIGHT/yScale, title, nullptr, nullptr);
}
