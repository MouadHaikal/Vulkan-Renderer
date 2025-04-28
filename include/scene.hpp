#pragma once

#include <textureManager.hpp>
#include <sceneObject.hpp>


class Scene{
public:
    static void setContext(const VulkanContext& ctx);

    void addTexture(const std::string& name, const std::string& path);
    void addObject(std::variant<std::string, RawMesh> source, const std::string& textureName = "None", glm::vec3 albedoColor = defaultColor);

    size_t      getTextureCount() const;
    VkImageView getTextureImageView(size_t index) const;

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;

    void cleanup();

private:
    static VulkanContext     context;

    TextureManager           textureManager;
    std::vector<SceneObject> objects;
};
