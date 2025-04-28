#pragma once

#include <mesh.hpp>
#include <material.hpp>


class SceneObject{
public:
    SceneObject(std::variant<std::string, RawMesh> source, int textureIndex = -1, glm::vec3 albedoColor = defaultColor);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;

    void cleanup();

private:

    Mesh     mesh;
    Material material;
};
