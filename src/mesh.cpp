#include <mesh.hpp>

#include <logger.hpp>


VulkanContext Mesh::context;


void Mesh::setContext(const VulkanContext& ctx){ context = ctx; }


Mesh::Mesh(const RawMesh* source, glm::mat4* modelMatrix) : modelMatrix(modelMatrix) {
    indexCount = source->indices.size();

    createVertexBuffer(source->vertices);
    createIndexBuffer(source->indices);

    material = source->material;


    LOG_DEBUG_S("Mesh loaded (" << indexCount/3 << " triangles)");
}


void Mesh::setMaterial(const Material& material){
    this->material = material;
}


void Mesh::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const{
    VertexPushConstant vertexPC;
    vertexPC.modelMatrix = *modelMatrix;

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexPushConstant), &vertexPC);


    material.pushInfo(commandBuffer, pipelineLayout);


    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
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
                        context.physicalDevice, context.device, context.queueFamilyIndices
    );

    void* data;
    vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(context.device, stagingBufferMemory);

    utils::createBuffer("vertex",
                        bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        vertexBuffer, vertexBufferMemory,
                        context.physicalDevice, context.device, context.queueFamilyIndices
    );

    utils::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, context.device, context.transferQueue, context.transferCommandPool);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);
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
                        context.physicalDevice, context.device, context.queueFamilyIndices
    );

    void* data;
    vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(context.device, stagingBufferMemory);

    utils::createBuffer("index",
                        bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        indexBuffer, indexBufferMemory,
                        context.physicalDevice, context.device, context.queueFamilyIndices
    );

    utils::copyBuffer(stagingBuffer, indexBuffer, bufferSize, context.device, context.transferQueue, context.transferCommandPool);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}


void Mesh::cleanup(){
    vkDestroyBuffer(context.device, indexBuffer, nullptr);
    vkFreeMemory(context.device, indexBufferMemory, nullptr);

    vkDestroyBuffer(context.device, vertexBuffer, nullptr);
    vkFreeMemory(context.device, vertexBufferMemory, nullptr);
}
