#include <gui.hpp>

#include <logger.hpp>



std::unordered_set<VkPresentModeKHR> Gui::presentModes{
        VK_PRESENT_MODE_FIFO_KHR,
        VK_PRESENT_MODE_MAILBOX_KHR,
        VK_PRESENT_MODE_IMMEDIATE_KHR
};


Gui::Gui() : currPresentMode(VK_PRESENT_MODE_FIFO_KHR), stateFlags(0){
    for (const auto& mode : presentModes) {
        availablePresentModes[mode] = false;
    }
}


void Gui::init(GuiInitInfo* info, ImGui_ImplVulkan_InitInfo* ImGuiInfo){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
                     ImGuiConfigFlags_DockingEnable;

    style();

    ImGui_ImplGlfw_InitForVulkan(info->window, true);
    ImGui_ImplVulkan_Init(ImGuiInfo);


    msaaSampleCount  = info->msaaSampleCount;
    minSampleShading = info->minSampleShading;
    scene            = info->scene;
    camera           = info->camera;

    // Present Modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ImGuiInfo->PhysicalDevice, info->surface, &presentModeCount, nullptr);

    std::vector<VkPresentModeKHR> supportedPresentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ImGuiInfo->PhysicalDevice, info->surface, &presentModeCount, supportedPresentModes.data());

    for (const auto& mode : supportedPresentModes) {
        if (presentModes.find(mode) != presentModes.end()) {
            availablePresentModes.at(mode) = true;
        }
    }

    // Device Info
    vkGetPhysicalDeviceProperties(ImGuiInfo->PhysicalDevice, &physicalDeviceProperties);
}


void Gui::build(){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static ImGuiIO&    io    = ImGui::GetIO(); (void)io;
    static ImGuiStyle& style = ImGui::GetStyle();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(500, 0), ImGuiCond_Once);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    ImGui::Begin("Performance");
        static float t = 0;
        t += io.DeltaTime;

        ImGui::Text("Device : %s", physicalDeviceProperties.deviceName);
        
        if (ImGui::BeginTable("Metrics", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Frame rate");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f", io.Framerate);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Frame time");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f ms", 1000.f * io.DeltaTime);
        
            ImGui::EndTable();
        }

        if (ImGui::BeginCombo("Present mode", utils::toCstr(currPresentMode))) {
            bool isSelected, isDisabled;
            
            for (const auto& mode : presentModes) {
                if ((isDisabled = !availablePresentModes.at(mode))) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                }

                isSelected = (mode == currPresentMode);

                if (ImGui::Selectable(utils::toCstr(mode), isSelected) && !isSelected) {
                    currPresentMode = mode;
                    stateFlags |= STATE_CHANGED_PRESENT_MODE;
                }

                if (isSelected) ImGui::SetItemDefaultFocus();

                if (ImGui::IsItemHovered()) {
                    const char* tooltip;
                    switch (mode) {
                        case VK_PRESENT_MODE_FIFO_KHR: 
                            tooltip = "No tearing - Limited frame rate (VSync)";
                            break;
                        case VK_PRESENT_MODE_MAILBOX_KHR:
                            tooltip = "No tearing - Unlimited frame rate";
                            break;
                        case VK_PRESENT_MODE_IMMEDIATE_KHR:
                            tooltip = "May result in visible tearing - Unlimited frame rate";
                            break;
                        default:  tooltip = "N/A";
                    }
                    ImGui::SetItemTooltip("%s", tooltip);
                }

                if (isDisabled) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleColor(); 
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("MSAA", utils::toCstr(*msaaSampleCount))) {
            bool isSelected;

            for (VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT; 
                sampleCount <= utils::getMaxUsableSampleCount(physicalDeviceProperties); 
                sampleCount = static_cast<VkSampleCountFlagBits>(sampleCount << 1)
            ) {

                isSelected = (sampleCount == *msaaSampleCount);

                if (ImGui::Selectable(utils::toCstr(sampleCount), isSelected) && !isSelected) {
                    *msaaSampleCount = sampleCount;
                    stateFlags |= STATE_CHANGED_SAMPLE_COUNT;
                }

                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        {
            ImGui::BeginDisabled(*msaaSampleCount == VK_SAMPLE_COUNT_1_BIT);
            if (ImGui::SliderFloat("Min sample shading", minSampleShading.get(), 0.f, 1.f, "%.2f")){
                stateFlags |= STATE_CHANGED_MIN_SAMPLE_SHADING;
            }
            ImGui::EndDisabled();
            helpMarker("Minimum fraction of sample shading - Requires MSAA");
        }
        
        ImGui::Spacing();
        ImGui::SeparatorText("Timeline");

        // Frame Rate Plot
        {
            ImVec4 fpsPlotCol{1.f, .8f, .15f, 1.f};

            static ScrollingBuffer fpsData(3000);
            static float fpsHistory = 10.f;

            static bool dynamicY = true;
            ImGui::Checkbox("Dynamic ordinate range", &dynamicY); 
            helpMarker("Dynamically alter Y-axis range - May result in some flickering");

            fpsData.addPoint(t, io.Framerate);

            if (ImPlot::BeginPlot("Frame Rate", ImVec2(-1, ImGui::GetFontSize() * 10))) {
                ImPlot::SetupAxes("Time", "Frame Rate", ImPlotAxisFlags_NoTickLabels, 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, t - fpsHistory, t, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 
                                        0, 
                                        ((int)(fpsData.getLastPoint().y / 100) + 1) * 100, 
                                        dynamicY ? ImGuiCond_Always : ImGuiCond_Once
                );

                ImPlot::SetNextFillStyle(fpsPlotCol, .2f);
                ImPlot::PlotShaded("FPS", 
                                   &fpsData.data[0].x, &fpsData.data[0].y, 
                                   fpsData.data.size(), -INFINITY, 0, 
                                   fpsData.offset, 2 * sizeof(float)
                                   );

                ImPlot::SetNextFillStyle(fpsPlotCol);
                ImPlot::PlotLine("FPS", 
                                 &fpsData.data[0].x, &fpsData.data[0].y, 
                                 fpsData.data.size(), 0, 
                                 fpsData.offset, 2 * sizeof(float)
                                 );

                ImPlot::EndPlot();
            } 

            ImGui::SliderFloat("History", &fpsHistory, 1, 10, "%.1f s");
        }
    ImGui::End();



    ImGui::SetNextWindowPos(ImVec2(550, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Once);
    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
    ImGui::Begin("Settings");
        ImGui::SeparatorText("UI");

        if(ImGui::SliderFloat("Font size", &style.FontSizeBase, 6, 40, "%.0f"))
            style._NextFrameFontSizeBase = style.FontSizeBase;

        ImGui::Spacing();
        ImGui::SeparatorText("Scene");
        if (ImGui::CollapsingHeader("Models")) {
        }
        
        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::TextDisabled("Move and look around using WASD+E+Space and the mouse");
            ImGui::TextDisabled("Hit Escape to toggle focus between the scene and the ui");

            ImGui::InputFloat3("Position", glm::value_ptr(camera->position));
            ImGui::SameLine(); 
            if (ImGui::Button("Reset")) camera->position = glm::vec3(0.f);

            ImGui::InputFloat("Speed", &camera->movementSpeed);
            ImGui::DragFloat2("Sensitivity", glm::value_ptr(camera->mouseSensitivity), 0.5f, 0.5f, 50.f);
            ImGui::DragFloat("FOV", &camera->fov, 1.f, 10.f, 140.f);
        }

        if (ImGui::CollapsingHeader("Lighting")) {
            ImGui::Spacing();
            ImGui::SeparatorText("Point lights");
            
            int idx = 1;
            for (auto it = scene->pointLights.begin(); it != scene->pointLights.end(); ) {
                ImGui::PushID(idx);    

                if (ImGui::TreeNode("##PL", "Point light %d", idx)) {
                    ImGui::ColorEdit3("Color", glm::value_ptr(it->color));
                    ImGui::DragFloat3("Position", glm::value_ptr(it->position), 10.f);
                    ImGui::DragFloat("Intensity", &it->intensity, 100.f);
                    
                    if (ImGui::SmallButton("Remove##PL")) {
                        stateFlags |= STATE_CHANGED_LIGHT_COUNT;
                        it = scene->pointLights.erase(it);
                        ImGui::TreePop();
                        ImGui::PopID();
                        continue;
                    } 

                    ImGui::TreePop();
                }
            
                ImGui::PopID();
                ++it;
                ++idx;
            }
            if (ImGui::Button("Add##PL")) {
                scene->pointLights.emplace_back();
                stateFlags |= STATE_CHANGED_LIGHT_COUNT;
            }
            
            ImGui::Spacing();
            ImGui::SeparatorText("Directional lights");

            idx = 1;
            for (auto it = scene->directionalLights.begin(); it != scene->directionalLights.end(); ) {
                ImGui::PushID(idx);
                if (ImGui::TreeNode("##DL", "Directional light %d", idx)) {
                    ImGui::ColorEdit3("Color", glm::value_ptr(it->color));
                    ImGui::DragFloat3("Direction", glm::value_ptr(it->direction), 10.f);
                    ImGui::DragFloat("Normal irradiance", &it->normalIrradiance, .001f);

                    if (ImGui::SmallButton("Remove##DL")) {
                        stateFlags |= STATE_CHANGED_LIGHT_COUNT;
                        it = scene->directionalLights.erase(it);
                        ImGui::TreePop();
                        ImGui::PopID();
                        continue;
                    } 

                    ImGui::TreePop();
                }

                ImGui::PopID();
                ++it;
                ++idx;
            }

            if (ImGui::Button("Add##DL")) {
                scene->directionalLights.emplace_back();
                stateFlags |= STATE_CHANGED_LIGHT_COUNT;
            }
        }
    ImGui::End();
}

void Gui::draw(VkCommandBuffer commandBuffer){
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}


bool Gui::shouldRecreateRenderPass(){
    if (stateFlags & STATE_CHANGED_SAMPLE_COUNT) {
        stateFlags &= ~STATE_CHANGED_SAMPLE_COUNT;
        return true;
    }

    return false;
}

bool Gui::shouldRecreateSwapchain(){
    if (stateFlags & STATE_CHANGED_PRESENT_MODE) {
        stateFlags &= ~STATE_CHANGED_PRESENT_MODE;
        return true;
    }

    return false;
}

bool Gui::shouldRecreateGraphicsPipeline(){
    if (stateFlags & STATE_CHANGED_MIN_SAMPLE_SHADING) {
        stateFlags &= ~STATE_CHANGED_MIN_SAMPLE_SHADING;
        return true;
    }

    return false;
}

bool Gui::shouldRecreateUniformBuffers(){
    if (stateFlags & STATE_CHANGED_LIGHT_COUNT) {
        stateFlags &= ~STATE_CHANGED_LIGHT_COUNT;
        return true;
    }

    return false;
}

void Gui::setMinImageCount(uint32_t count){ ImGui_ImplVulkan_SetMinImageCount(count); }


VkPresentModeKHR Gui::getCurrPresentMode() const{ return currPresentMode; }


void Gui::cleanup(){
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}



void Gui::style(){
    ImGui::StyleColorsDark();

    ImGuiIO& io       = ImGui::GetIO(); (void)io;
    ImGuiStyle& style = ImGui::GetStyle();

    // - Font
    ImFont* font = io.Fonts->AddFontFromFileTTF((dirFonts/"Rubik-Regular.ttf").string().c_str(), 15.f);
    if (!font) LOG_WARNING("Failed to load font");

    // - Spacing/Padding
    style.WindowPadding    = ImVec2(12.f, 12.f);
    style.FramePadding     = ImVec2(12.f, 4.f);
    style.ItemSpacing      = ImVec2(8.f, 8.f);
    style.ItemInnerSpacing = ImVec2(4.f, 4.f);
    style.IndentSpacing    = 20.f;
    style.ScrollbarSize    = 10.f;
    style.GrabMinSize      = 10.f;

    // - Rounding
    style.WindowRounding    = 6.f;
    style.ChildRounding     = 6.f;
    style.FrameRounding     = 4.f;
    style.GrabRounding      = 4.f;
    style.PopupRounding     = 6.f;
    style.ScrollbarRounding = 6.f;
    style.TabRounding       = 4.0f;

    // - Trees
    style.TreeLinesFlags    = ImGuiTreeNodeFlags_DrawLinesFull;
    style.TreeLinesSize     = 1.f;
    style.TreeLinesRounding = 3.f;

    // - Border
    style.FrameBorderSize = 1.f;
    style.PopupBorderSize = 1.f;

    // - Hover
    style.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_Stationary;
    style.HoverFlagsForTooltipNav   = ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_NoSharedDelay;


    // - Colors
    ImVec4* colors = ImGui::GetStyle().Colors;


    colors[ImGuiCol_Text]                       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]               = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]                   = ImVec4(0.14f, 0.14f, 0.14f, 0.94f);
    colors[ImGuiCol_ChildBg]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.31f);
    colors[ImGuiCol_PopupBg]                    = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border]                     = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_BorderShadow]               = ImVec4(0.20f, 0.20f, 0.20f, 0.39f);
    colors[ImGuiCol_FrameBg]                    = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_FrameBgActive]              = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBg]                    = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive]              = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]           = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]                  = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]                = ImVec4(0.35f, 0.35f, 0.35f, 0.59f);
    colors[ImGuiCol_ScrollbarGrab]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]       = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_CheckMark]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]                 = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]           = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_Button]                     = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered]              = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ButtonActive]               = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_Header]                     = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_HeaderHovered]              = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_HeaderActive]               = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_Separator]                  = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]           = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_SeparatorActive]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_ResizeGrip]                 = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_InputTextCursor]            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabHovered]                 = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_Tab]                        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabSelected]                = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]        = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_TabDimmed]                  = ImVec4(0.20f, 0.20f, 0.20f, 0.78f);
    colors[ImGuiCol_TabDimmedSelected]          = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_DockingPreview]             = ImVec4(0.39f, 0.39f, 0.39f, 0.78f);
    colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]                  = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]           = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]              = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]              = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]          = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_TableBorderLight]           = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TableRowBg]                 = ImVec4(0.04f, 0.04f, 0.04f, 0.39f);
    colors[ImGuiCol_TableRowBgAlt]              = ImVec4(0.24f, 0.24f, 0.24f, 0.39f);
    colors[ImGuiCol_TextLink]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]             = ImVec4(0.39f, 0.39f, 0.39f, 0.39f);
    colors[ImGuiCol_TreeLines]                  = ImVec4(0.39f, 0.39f, 0.39f, 0.78f);
    colors[ImGuiCol_DragDropTarget]             = ImVec4(0.39f, 0.39f, 0.39f, 0.78f);
    colors[ImGuiCol_NavCursor]                  = ImVec4(0.39f, 0.39f, 0.39f, 0.78f);
    colors[ImGuiCol_NavWindowingHighlight]      = ImVec4(0.39f, 0.39f, 0.39f, 0.78f);
    colors[ImGuiCol_NavWindowingDimBg]          = ImVec4(0.35f, 0.35f, 0.35f, 0.27f);
    colors[ImGuiCol_ModalWindowDimBg]           = ImVec4(0.35f, 0.35f, 0.35f, 0.27f);
}

void Gui::helpMarker(const char* desc){
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
}



Gui::ScrollingBuffer::ScrollingBuffer(int maxSize) : maxSize(maxSize), offset(0){ data.reserve(maxSize); }

void Gui::ScrollingBuffer::addPoint(float x, float y){
    if (data.size() < maxSize)
        data.push_back(ImVec2(x,y));
    else {
        data[offset] = ImVec2(x,y);
        offset = (offset + 1) % maxSize;
    }
}

ImVec2 Gui::ScrollingBuffer::getLastPoint(){
    return data.size() < maxSize ? 
        (data.size() ? data.back()    : ImVec2(0.f, 0.f)) : 
        (offset      ? data[offset-1] : data.back());
}

void Gui::ScrollingBuffer::erase(){
    if (data.size() > 0) {
        data.shrink(0);
        offset  = 0;
    }
}
