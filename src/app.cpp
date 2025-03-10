#include "app.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <atomic>
#include <thread>

// Global atomic flag to signal window closure
std::atomic<bool> closeRequested{false};

// Main Functions
void App::run(){
    init();

    // Start a thread to wait for "x" input on the terminal
    std::thread inputThread([](){
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "x") {
                closeRequested.store(true);
                break;
            }
        }
    });

    mainLoop();

    inputThread.join(); // Wait for the input thread to finish

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
        if (closeRequested.load()) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        glfwPollEvents();

        // Optionally sleep briefly to avoid busy waiting
        /*std::this_thread::sleep_for(std::chrono::milliseconds(10));*/
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
