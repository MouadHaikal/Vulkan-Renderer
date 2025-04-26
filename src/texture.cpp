#include <texture.hpp>

#include <logger.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>



Texture::Texture(const TextureData& textureData) :
    physicalDevice(textureData.physicalDevice), device(textureData.device),
    queueFamilyIndices(textureData.queueFamilyIndices),
    graphicsCommandPool(textureData.graphicsCommandPool), graphicsQueue(textureData.graphicsQueue),
    transferCommandPool(textureData.transferCommandPool), transferQueue(textureData.transferQueue)
{
    createTextureImage(textureData.imagePath);
    createTextureImageView();
}

VkImageView Texture::getImageView() const{ return textureImageView; }

void Texture::cleanup(){
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
}


void Texture::createTextureImage(const std::string& imagePath){
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) LOG_FATAL("Failed to load texture image");

    VkDeviceSize imageSize = texWidth * texHeight * 4;


    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    utils::createBuffer("staging",
                        imageSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory,
                        physicalDevice, device, queueFamilyIndices
    );


    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    
    utils::createImage("texture", 
                       texWidth, texHeight, 
                       VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       textureImage, textureImageMemory,
                       physicalDevice, device, queueFamilyIndices
    );


    utils::transitionImageLayout(textureImage, 
                                 VK_FORMAT_R8G8B8A8_SRGB, 
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 device, graphicsQueue, graphicsCommandPool
    );

    utils::copyBufferToImage(stagingBuffer, textureImage,
                             static_cast<uint32_t>(texWidth),
                             static_cast<uint32_t>(texHeight),
                             device, transferQueue, transferCommandPool
    );

    utils::transitionImageLayout(textureImage,
                                 VK_FORMAT_R8G8B8A8_SRGB,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 device, graphicsQueue, graphicsCommandPool
    );


    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Texture::createTextureImageView(){
    utils::createImageView("texture", 
                           textureImage, 
                           VK_FORMAT_R8G8B8A8_SRGB,
                           VK_IMAGE_ASPECT_COLOR_BIT,
                           textureImageView, device
    );
}
