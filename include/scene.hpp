#pragma once

#include <textureManager.hpp>
#include <model.hpp>


class Scene{
public:
    TextureManager textureManager;

    std::vector<PointLight>       pointLights;
    std::vector<DirectionalLight> directionalLights;


    static void setContext(const VulkanContext& ctx);


    std::shared_ptr<Model> addModel(std::variant<std::string, RawMesh*> source);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;

    void cleanup();

private:
    static VulkanContext context;

    std::vector<std::shared_ptr<Model>> models;
};
