#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb/stb_image.h>


#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <functional>
#include <cstddef>
#include <cstdint>
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
const std::filesystem::path dirFonts    = dirRoot / "assets" / "fonts";



struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete();
};


struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    // std::vector<VkPresentModeKHR>   presentModes;
};


struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    static VkVertexInputBindingDescription                  getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();
};


struct VPubo {
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

enum TextureType{
    TEX_TYPE_BASE,
    TEX_TYPE_NORMAL,

    TEX_TYPE_ENUM_SIZE
};

constexpr std::array<VkFormat, TextureType::TEX_TYPE_ENUM_SIZE> textureFormats{
    VK_FORMAT_R8G8B8A8_SRGB,    // Base
    VK_FORMAT_R8G8B8A8_UNORM    // Normal
};

struct VertexPushConstant {
    alignas(16) glm::mat4 modelMatrix;
};

struct FragmentPushConstant {
    alignas(16) glm::vec3 albedoColor;
    alignas(4)  int       baseTextureIndex;
    alignas(16) glm::vec3 cameraPos;
    alignas(4)  int       normalMapIndex;
};


struct PointLight{
    alignas(16) glm::vec3 position  = glm::vec3(0.f);
    alignas(4)  float     intensity = 1000.f;
    alignas(16) glm::vec3 color     = glm::vec3(1.f);
};

struct DirectionalLight{
    alignas(16) glm::vec3 direction        = glm::vec3(0.f, 0.f, -1.f);
    alignas(4)  float     normalIrradiance = .1f;
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

    // Other
    std::string getFileName(const std::string& path);
    
    VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDeviceProperties deviceProperties);

    const char* toCstr(VkPresentModeKHR presentMode);
    const char* toCstr(VkSampleCountFlagBits sampleCount);
}
