#pragma once

#include <renderer.hpp>


// Window resolution in screen coordinates
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

class App {
public: 
    void run();

private:

    GLFWwindow * window;
    Renderer     renderer;
    InputHandler inputHandler;

    float deltaTime    = 0.f;
    float lastFrame    = 0.f;


    void init();
    void mainLoop();
    void cleanup();


    void initWindow(const char* title);
    static void framebufferResizeCallback(GLFWwindow * window, int width, int height);
};
