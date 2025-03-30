#include "app.hpp"
#include <iostream>

// Potential Problem:
// ** The size of the window is not always WIDTH x HEIGHT due to scale (see app.cpp)
//
// TODO: Set Application/Engine name in Renderer::createVulkanInstance() and App::init()
// TODO: Edit Renderer::rateDeviceSuitability(VkPhysicalDevice device)
// TODO: Set build config macro (App::init())

int main(){
    App app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
