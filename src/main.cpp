#include "app.hpp"

// Potential Problem:
// ** The size of the window is not always WIDTH x HEIGHT due to scale (see app.cpp)
//
// TODO: Set Application/Engine name in Renderer::createVulkanInstance() and App::init()
// TODO: Edit Renderer::rateDeviceSuitability(VkPhysicalDevice device)
// TODO: Set build config macro (see App::init() & Renderer::init())

int main(){
    App app;
    app.run();
}
