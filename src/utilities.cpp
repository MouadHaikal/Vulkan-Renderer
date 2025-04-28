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

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions(){
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Vertex, pos);

    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const {
    return pos      == other.pos   &&
           texCoord == other.texCoord;
}

namespace std {
    size_t hash<Vertex>::operator()(const Vertex& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos)                   ^
                (hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1);
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

void utils::transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkDevice device, VkQueue queue, VkCommandPool commandPool
){
    VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(commandPool, device);

        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = oldLayout;
        barrier.newLayout           = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = image;


        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        ){
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } 
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        ){
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } 
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                 newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } 
        else {
            LOG_ERROR("Unsupported layout transition");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    utils::endSingleTimeCommands(commandBuffer, device, commandPool, queue);
}

bool utils::hasStencilComponent(VkFormat format){
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void utils::copyBufferToImage(VkBuffer buffer, VkImage image, 
                              uint32_t width, uint32_t height, VkDevice device, 
                              VkQueue queue, VkCommandPool commandPool){
    VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(commandPool, device);

        VkBufferImageCopy region{};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    utils::endSingleTimeCommands(commandBuffer, device, commandPool, queue);
}

void utils::createImageView(const std::string& name, 
                            VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, 
                            VkImageView& imageView, VkDevice device){
    VkImageViewCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format   = format;

    createInfo.subresourceRange.aspectMask     = aspectFlags;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    LOG_RESULT(
        vkCreateImageView(device, &createInfo, nullptr, &imageView),
        "Create " + name + " image view"
    );
}
