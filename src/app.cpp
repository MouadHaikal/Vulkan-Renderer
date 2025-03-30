#include "app.hpp"
#include "logger.hpp"

#include <cstdlib>
#include <iostream>

// Main Functions
void App::run(){
    init();

    mainLoop();

    cleanup();
}

void App::init(){
    Logger::get().setMinLevel(Logger::Level::DEBUG);

    glfwInit();
    initWindow("VulkanApp");

    if (renderer.init(window) != EXIT_SUCCESS) {
        throw std::runtime_error("Failed to initialize renderer!");
    }
}

void App::mainLoop(){
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.drawFrame();
    }

    renderer.deviceWait();
}

void App::cleanup(){
    renderer.cleanup();

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
        std::cout << "Accounting for monitor scale: " << xScale << "x" << yScale << std::endl;
    } else {
        std::cerr << "Failed to get primary monitor! Ignoring monitor scale!" << std::endl;
        xScale = 1.0f;
        yScale = 1.0f;
    }

    window = glfwCreateWindow(WIDTH/xScale, HEIGHT/yScale, title, nullptr, nullptr);
}
