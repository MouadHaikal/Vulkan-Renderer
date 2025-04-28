#pragma once

#include <utilities.hpp>


using RawMesh = std::pair<std::vector<Vertex>, std::vector<uint32_t>>;


class Mesh{
public:
    static void setContext(const VulkanContext& ctx);


    Mesh(std::variant<std::string, RawMesh> source);

    void pushInfo(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;

    void draw(VkCommandBuffer commandBuffer) const;

    void cleanup();
        
private:
    static VulkanContext context;

    glm::mat4          modelMatrix = glm::mat4(1.0f);


    VkBuffer           vertexBuffer;
    VkDeviceMemory     vertexBufferMemory;

    VkBuffer           indexBuffer;
    VkDeviceMemory     indexBufferMemory;

    size_t             indexCount = 0;
    

    size_t loadModel(const std::string& modelPath);

    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
};
