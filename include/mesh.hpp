#pragma once

#include <utilities.hpp>
#include <material.hpp>


struct RawMesh {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    Material              material;
};


class Mesh{
public:
    static void setContext(const VulkanContext& ctx);


    Mesh(const RawMesh* source, glm::mat4* modelMatrix);

    void setMaterial(const Material& material);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;

    void cleanup();
        
private:
    static VulkanContext context;

    glm::mat4* const   modelMatrix;
    Material           material;

    VkBuffer           vertexBuffer;
    VkDeviceMemory     vertexBufferMemory;

    VkBuffer           indexBuffer;
    VkDeviceMemory     indexBufferMemory;

    size_t             indexCount = 0;
    
    

    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
};
