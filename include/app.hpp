#pragma once

#include "renderer.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class App {
public: 
    void run();

private:
    // Variables
    GLFWwindow * window;
    Renderer renderer;


    // Main Functions
    void init();
    void mainLoop();
    void cleanUp();
    
    // Helper Functions
    void initWindow(const char* title);
};
