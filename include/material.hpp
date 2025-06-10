#pragma once

#include <utilities.hpp>


constexpr glm::vec3 defaultColor = glm::vec3(.3f);

class Material{
public:
    Material(int textureIndex = -1, glm::vec3 albedoColor = defaultColor);

    void setTextureIndex(int index);

    void pushInfo(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const glm::vec3& cameraPos) const;

private:

    glm::vec3 albedoColor;
    int       textureIndex;
};
