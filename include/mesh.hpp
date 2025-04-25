#pragma once

#include <utilities.hpp>


using RawMesh = std::pair<std::vector<Vertex>, std::vector<uint32_t>>;

struct MeshData {
    std::variant<std::string, RawMesh> source;

    VkPhysicalDevice physicalDevice;
    VkDevice         device;

    QueueFamilyIndices queueFamilyIndices;

    VkQueue          transferQueue;
    VkCommandPool    transferCommandPool;
};

class Mesh{
public:
    Mesh(const MeshData& meshData);

    void draw(VkCommandBuffer commandBuffer) const;

    void cleanup();
        
private:
    VkPhysicalDevice   physicalDevice;
    VkDevice           device;

    QueueFamilyIndices queueFamilyIndices;

    VkQueue            transferQueue;
    VkCommandPool      transferCommandPool;

    VkBuffer           vertexBuffer;
    VkDeviceMemory     vertexBufferMemory;

    VkBuffer           indexBuffer;
    VkDeviceMemory     indexBufferMemory;

    uint32_t           indexCount;

    glm::mat4          modelMatrix;     // Currently unused


    uint32_t loadModel(const std::string& modelPath);

    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
};
