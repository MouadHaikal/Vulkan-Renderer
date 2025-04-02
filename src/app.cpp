#include "app.hpp"
#include "logger.hpp"


// Main Functions
void App::run(){
    init();

    mainLoop();

    cleanup();
}

void App::init(){
    Logger::get().setMinLevel(Logger::Level::TRACE);

    glfwInit();

    initWindow("VulkanApp");

    renderer.init(window);
}

void App::mainLoop(){
    LOG_DEBUG("Entering main loop");

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

    Logger::get().destroy();
}


// Helper Functions
void App::initWindow(const char* title){
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, title, nullptr, nullptr);

    glfwSetWindowUserPointer(window, &renderer);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void App::framebufferResizeCallback(GLFWwindow * window, int width, int height){
    auto pRenderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    pRenderer->framebufferResized = true;
}
