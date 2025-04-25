#include <mesh.hpp>

#include <logger.hpp>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tol/tiny_obj_loader.h>



Mesh::Mesh(const MeshData& meshData) : 
    physicalDevice(meshData.physicalDevice), device(meshData.device), queueFamilyIndices(meshData.queueFamilyIndices),
    transferQueue(meshData.transferQueue), transferCommandPool(meshData.transferCommandPool)
{
    if (std::holds_alternative<RawMesh>(meshData.source)) {
        const auto& [vertices, indices] = std::get<RawMesh>(meshData.source);  

        indexCount = static_cast<uint32_t>(indices.size());

        createVertexBuffer(vertices);
        createIndexBuffer(indices);

    } else {
        const std::string& modelPath = std::get<std::string>(meshData.source);

        indexCount = loadModel(modelPath);
    }

    LOG_TRACE_S("Mesh loaded (" << indexCount << " indices)");
}

void Mesh::draw(VkCommandBuffer commandBuffer) const{
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}

void Mesh::cleanup(){
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
}


uint32_t Mesh::loadModel(const std::string& modelPath){
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;


    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str())) {
        LOG_ERROR_S("Failed to load model : " << warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };


            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }


    createVertexBuffer(vertices);
    createIndexBuffer(indices);

    return static_cast<uint32_t>(indices.size());
}

void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices){
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    utils::createBuffer("staging",
                        bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory,
                        physicalDevice, device, queueFamilyIndices
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    utils::createBuffer("vertex",
                        bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        vertexBuffer, vertexBufferMemory,
                        physicalDevice, device, queueFamilyIndices
    );

    utils::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, device, transferQueue, transferCommandPool);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices){
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    utils::createBuffer("staging",
                        bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory,
                        physicalDevice, device, queueFamilyIndices
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    utils::createBuffer("index",
                        bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        indexBuffer, indexBufferMemory,
                        physicalDevice, device, queueFamilyIndices
    );

    utils::copyBuffer(stagingBuffer, indexBuffer, bufferSize, device, transferQueue, transferCommandPool);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

