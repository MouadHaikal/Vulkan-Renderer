#pragma once


#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <implot.h>

#include <utilities.hpp>
#include <camera.hpp>


struct GuiInitInfo{
    GLFWwindow*             window;
    VkSurfaceKHR            surface;
    VkSampleCountFlagBits   sampleCount;
    float                   minSampleShading;
    std::shared_ptr<Camera> camera;
};

class Gui{
public:
    Gui();

    void init(GuiInitInfo* info, ImGui_ImplVulkan_InitInfo* ImGuiInfo);

    void build();
    void draw(VkCommandBuffer commandBuffer);

    bool shouldRecreateRenderPass(VkSampleCountFlagBits* sampleCount);
    bool shouldRecreateSwapchain();
    bool shouldRecreateGraphicsPipeline(float* minSampleShading);

    void setMinImageCount(uint32_t count);

    VkPresentModeKHR getCurrPresentMode() const;

    void cleanup();
private:
    enum StateFlagBits{
        STATE_CHANGED_PRESENT_MODE       = 1 << 0,
        STATE_CHANGED_SAMPLE_COUNT       = 1 << 1,
        STATE_CHANGED_MIN_SAMPLE_SHADING = 1 << 2
    };

    uint32_t stateFlags;
    VkPhysicalDeviceProperties physicalDeviceProperties;

    static std::unordered_set<VkPresentModeKHR> presentModes;

    std::unordered_map<VkPresentModeKHR, bool> availablePresentModes;
    VkPresentModeKHR                           currPresentMode;
    VkSampleCountFlagBits                      currSampleCount;
    float                                      currMinSampleShading;
    std::shared_ptr<Camera>                    camera;


    static void style();

    static void helpMarker(const char* desc);


    struct ScrollingBuffer {
        int maxSize;
        int offset;
        ImVector<ImVec2> data;

        ScrollingBuffer(int maxSize = 2000);

        void addPoint(float x, float y);
        ImVec2 getLastPoint();
        void erase();
    };
};
