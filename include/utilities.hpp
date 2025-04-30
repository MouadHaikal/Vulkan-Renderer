#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <bits/stdc++.h>


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete();
};


struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};


struct Vertex {
    glm::vec3 pos;
    // glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const;

    static VkVertexInputBindingDescription                  getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(const Vertex& vertex) const;
    };
}


struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


struct VulkanContext {
    VkPhysicalDevice   physicalDevice;
    VkDevice           device;

    QueueFamilyIndices queueFamilyIndices;

    VkQueue            graphicsQueue;
    VkCommandPool      graphicsCommandPool;

    VkQueue            transferQueue;
    VkCommandPool      transferCommandPool;
};

struct VertexPushConstant {
    alignas(16) glm::mat4 modelMatrix;
};

struct FragmentPushConstant {
    alignas(4)  int       textureIndex;
    alignas(16) glm::vec3 albedoColor;
};


namespace utils {

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device);
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkCommandPool commandPool, VkQueue queue);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);

    bool hasStencilComponent(VkFormat format);


    // Create operations
    void createBuffer(const std::string& name, 
                      VkDeviceSize size, 
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, 
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory,
                      VkPhysicalDevice physicalDevice, VkDevice device, 
                      const QueueFamilyIndices& indices);

    void createImage(const std::string& name, 
                     uint32_t width, uint32_t height, 
                     VkFormat format, VkImageTiling tiling, 
                     VkImageUsageFlags usage, 
                     VkMemoryPropertyFlags properties, 
                     VkImage& image, VkDeviceMemory& imageMemory,
                     VkPhysicalDevice physicalDevice, VkDevice device,
                     const QueueFamilyIndices& indices);
    void createImageView(const std::string& name, 
                         VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, 
                         VkImageView& imageView, VkDevice device);


    // Transfer operations
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDevice device, VkQueue queue, VkCommandPool commandPool);
    void copyBufferToImage(VkBuffer buffer, VkImage image, 
                           uint32_t width, uint32_t height, VkDevice device, 
                           VkQueue queue, VkCommandPool commandPool);

    // Graphics operations
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkDevice device, VkQueue queue, VkCommandPool commandPool);

}
