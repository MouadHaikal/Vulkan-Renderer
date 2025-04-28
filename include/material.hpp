#pragma once

#include <utilities.hpp>


constexpr glm::vec3 defaultColor = glm::vec3(0.3);

class Material{
public:
    Material(int textureIndex = -1, glm::vec3 albedoColor = defaultColor);

    void pushInfo(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;

private:

    glm::vec3 albedoColor;
    int       textureIndex;
};
