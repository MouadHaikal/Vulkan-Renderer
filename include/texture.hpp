#pragma once

#include <utilities.hpp>


struct TextureData{
    std::string        imagePath;

    VkPhysicalDevice   physicalDevice;
    VkDevice           device;

    QueueFamilyIndices queueFamilyIndices;

    VkQueue            graphicsQueue;
    VkCommandPool      graphicsCommandPool;

    VkQueue            transferQueue;
    VkCommandPool      transferCommandPool;
};


class Texture{
public:
    Texture(const TextureData& textureData);

    VkImageView getImageView() const;

    void cleanup();

private:
    VkPhysicalDevice   physicalDevice;
    VkDevice           device;

    QueueFamilyIndices queueFamilyIndices;

    VkQueue            graphicsQueue;
    VkCommandPool      graphicsCommandPool;

    VkQueue            transferQueue;
    VkCommandPool      transferCommandPool;

    VkImage            textureImage       = VK_NULL_HANDLE;
    VkDeviceMemory     textureImageMemory = VK_NULL_HANDLE;
    VkImageView        textureImageView   = VK_NULL_HANDLE;


    void createTextureImage(const std::string& imagePath);
    void createTextureImageView();
};
