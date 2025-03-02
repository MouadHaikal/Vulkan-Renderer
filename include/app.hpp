#pragma once

#include "renderer.hpp"

class App {
public: 
    void run();

private:
    // Constants
    static const uint32_t WIDTH = 800;
    static const uint32_t HEIGHT = 600;

    const char* appName    = "VulkanApp";
    const char* engineName = "VulkanApp";

    // Main Properties
    GLFWwindow * window;
    Renderer renderer;


    // Main Functions
    void init();
    void mainLoop();
    void cleanUp();
    
    // Helper Functions
    void initWindow(const char* title);
};
