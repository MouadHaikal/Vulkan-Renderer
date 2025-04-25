#include <utilities.hpp>
#include <logger.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// QueueFamilyIndices ------------------------------------------------------------

bool QueueFamilyIndices::isComplete(){
    return graphicsFamily.has_value() &&
           presentFamily.has_value()  &&
           transferFamily.has_value();
}


// Vertex ------------------------------------------------------------------------

VkVertexInputBindingDescription Vertex::getBindingDescription(){
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding   = 0;
    bindingDescription.stride    = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions(){
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Vertex, pos);

    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Vertex, color);

    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const {
    return pos      == other.pos   &&
           color    == other.color &&
           texCoord == other.texCoord;
}

namespace std {
    size_t hash<Vertex>::operator()(const Vertex& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos)                ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
}


// Helper Functions --------------------------------------------------------------

uint32_t utils::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice){
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i=0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    LOG_FATAL("Failed to find suitable memory type");
    abort();      // Already called in LOG_FATAL();
}

VkCommandBuffer utils::beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device){
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void utils::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkCommandPool commandPool, VkQueue queue){
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void utils::createBuffer(const std::string& name,
                            VkDeviceSize size,
                            VkBufferUsageFlags usage,
                            VkMemoryPropertyFlags properties,
                            VkBuffer& buffer, VkDeviceMemory& bufferMemory,
                            VkPhysicalDevice physicalDevice, VkDevice device,
                            const QueueFamilyIndices& indices
){
    VkBufferCreateInfo createInfo{};
    createInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size        = size;
    createInfo.usage       = usage;

    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.transferFamily.value()
    };

    if (indices.transferFamily.value() != indices.graphicsFamily.value()) {
        createInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }


    LOG_RESULT(
        vkCreateBuffer(device, &createInfo, nullptr, &buffer),
        "Create " + name + " buffer"
    );


    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    LOG_RESULT(
        vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory),
        "Allocate " + name + " buffer memory"
    );


    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void utils::createImage(const std::string& name, 
                        uint32_t width, uint32_t height, 
                        VkFormat format, VkImageTiling tiling, 
                        VkImageUsageFlags usage, 
                        VkMemoryPropertyFlags properties,
                        VkImage& image, VkDeviceMemory& imageMemory,
                        VkPhysicalDevice physicalDevice, VkDevice device,
                        const QueueFamilyIndices& indices
){
    VkImageCreateInfo createInfo{};
    createInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType     = VK_IMAGE_TYPE_2D;
    createInfo.extent.width  = width;
    createInfo.extent.height = height;
    createInfo.extent.depth  = 1;
    createInfo.mipLevels     = 1;
    createInfo.arrayLayers   = 1;
    createInfo.format        = format;
    createInfo.tiling        = tiling;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage         = usage;
    createInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.transferFamily.value()
    };

    if (indices.transferFamily.value() != indices.graphicsFamily.value()) {
        createInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    LOG_RESULT(
        vkCreateImage(device, &createInfo, nullptr, &image), 
        "Create " + name + " image"
    );


    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    LOG_RESULT(
        vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory), 
        "Allocate " + name + " image memory"
    );

    vkBindImageMemory(device, image, imageMemory, 0);
}

void utils::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDevice device, VkQueue queue, VkCommandPool commandPool){
    LOG_TRACE("Copying buffer");

    VkCommandBuffer transferCommandBuffer = beginSingleTimeCommands(commandPool, device);

        VkBufferCopy copyRegion{};
        copyRegion.size      = size;

        vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(transferCommandBuffer, device, commandPool, queue);
}
