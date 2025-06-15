#include <app.hpp>

#include <logger.hpp>



void App::run(){
    init();

    mainLoop();

    cleanup();
}

void App::init(){
    Logger::get().minLevel = Logger::Level::DEBUG;

    glfwInit();

    initWindow("Vullkan Renderer");

    inputHandler.bindWindow(window);

    renderer.init(window);
}

void App::mainLoop(){
    LOG_DEBUG("Entering main loop");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        renderer.processInput(inputHandler.getInputData(), deltaTime);

        renderer.drawFrame();
    }
}

void App::cleanup(){
    renderer.cleanup();

    glfwDestroyWindow(window);

    glfwTerminate();

    Logger::get().destroy();
}


void App::initWindow(const char* title){
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, title, nullptr, nullptr);


    glfwSetWindowUserPointer(window, &renderer);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void App::framebufferResizeCallback(GLFWwindow * window, int width, int height){
    auto pRenderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    pRenderer->framebufferResized = true;
}
