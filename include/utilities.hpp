#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb/stb_image.h>



#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstddef>
#include <cstdint>
#include <unistd.h>
#include <memory>
#include <array>
#include <map>
#include <filesystem>
#include <optional>
#include <mutex>
#include <sstream>
#include <variant>
#include <set>
#include <cstring>
#include <fstream>


const std::filesystem::path dirRoot     = std::filesystem::path(__FILE__).parent_path().parent_path();
const std::filesystem::path dirShaders  = dirRoot / "shaders" / "spirv";
const std::filesystem::path dirModels   = dirRoot / "assets" / "models";
const std::filesystem::path dirTextures = dirRoot / "assets" / "textures";



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
    glm::vec2 texCoord;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const;

    static VkVertexInputBindingDescription                  getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(const Vertex& vertex) const;
    };
}


struct VPUBO {
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


struct PointLight{
    alignas(16) glm::vec3 position  = glm::vec3(0.f);
    alignas(4)  float     intensity = 100.f;
    alignas(16) glm::vec3 color     = glm::vec3(1.f);
};

struct DirectionalLight{
    alignas(16) glm::vec3 direction        = glm::vec3(0.f, 0.f, -1.f);
    alignas(4)  float     normalIrradiance = 50.f;
    alignas(16) glm::vec3 color            = glm::vec3(1.f);
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
                     uint32_t mipLevels, VkSampleCountFlagBits sampleCount,
                     VkFormat format, VkImageTiling tiling, 
                     VkImageUsageFlags usage, 
                     VkMemoryPropertyFlags properties, 
                     VkImage& image, VkDeviceMemory& imageMemory,
                     VkPhysicalDevice physicalDevice, VkDevice device,
                     const QueueFamilyIndices& indices);
    void createImageView(const std::string& name, 
                         VkImage image, VkFormat format,
                         uint32_t mipLevels, VkImageAspectFlags aspectFlags, 
                         VkImageView& imageView, VkDevice device);


    // Transfer operations
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDevice device, VkQueue queue, VkCommandPool commandPool);
    void copyBufferToImage(VkBuffer buffer, VkImage image, 
                           uint32_t width, uint32_t height, VkDevice device, 
                           VkQueue queue, VkCommandPool commandPool);

    // Graphics operations
    void transitionImageLayout(VkImage image, VkFormat format, uint32_t mipLevels,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkDevice device, VkQueue queue, VkCommandPool commandPool);

    std::string getFileName(const std::string& path);
}
