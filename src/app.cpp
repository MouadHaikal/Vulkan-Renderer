#include "app.hpp"
#include "logger.hpp"
#include "renderer.hpp"
#include <GLFW/glfw3.h>


// Main Functions
void App::run(){
    init();

    mainLoop();

    cleanup();
}

void App::init(){
    glfwInit();

    Logger::get().setMinLevel(Logger::Level::DEBUG);

    initWindow("VulkanApp");

    renderer.init(window);
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

    Logger::get().destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}


// Helper Functions
void App::initWindow(const char* title){
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    float xScale, yScale;
    
    if (monitor){
        glfwGetMonitorContentScale(monitor, &xScale, &yScale);
        LOG_INFO_S("Accounting for monitor scale: " << xScale << "x" << yScale);
    } else {
        LOG_ERROR("Failed to get primary monitor! Ignoring monitor scale!");
        xScale = 1.0f;
        yScale = 1.0f;
    }

    window = glfwCreateWindow(WIDTH/xScale, HEIGHT/yScale, title, nullptr, nullptr);

    glfwSetWindowUserPointer(window, &renderer);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void App::framebufferResizeCallback(GLFWwindow * window, int width, int height){
    auto pRenderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    pRenderer->framebufferResized = true;
}
