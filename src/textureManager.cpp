#include <textureManager.hpp>

#include <logger.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


VulkanContext TextureManager::context;


void TextureManager::setContext(const VulkanContext &ctx){ context = ctx; }


void TextureManager::addTexture(const std::string& name, const std::string& path){
    if (textureIndices.find(name) != textureIndices.end()) {
        LOG_WARNING_S("Texture '" << name << "' already loaded");
        return;
    }

    textureIndices[name] = static_cast<int>(textures.size());

    textures.push_back(createTexture(path));

    LOG_DEBUG_S("Texture loaded (" << name << ")");
}


int TextureManager::getTextureIndex(const std::string& name) const{
    if (textureIndices.find(name) == textureIndices.end()) {
        LOG_WARNING_S("Texture '" << name << "' not found");
        return -1;
    }

    return textureIndices.at(name);
}

size_t TextureManager::getTextureCount() const{
    return textures.size();
}
VkImageView TextureManager::getTextureImageView(size_t index) const{
    return textures[index].imageView;
}


void TextureManager::cleanup(){
    for (auto& texture : textures) {
        vkDestroyImageView(context.device, texture.imageView, nullptr);
        vkDestroyImage(context.device, texture.image, nullptr);
        vkFreeMemory(context.device, texture.imageMemory, nullptr);
    }
}


TextureManager::Texture TextureManager::createTexture(const std::string& imagePath){
    Texture texture;

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
                        context.physicalDevice, context.device, context.queueFamilyIndices
    );


    void* data;
    vkMapMemory(context.device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(context.device, stagingBufferMemory);

    stbi_image_free(pixels);

    
    utils::createImage("texture", 
                       texWidth, texHeight, 
                       VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       texture.image, texture.imageMemory,
                       context.physicalDevice, context.device, context.queueFamilyIndices
    );


    utils::transitionImageLayout(texture.image, 
                                 VK_FORMAT_R8G8B8A8_SRGB, 
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 context.device, context.graphicsQueue, context.graphicsCommandPool
    );

    utils::copyBufferToImage(stagingBuffer, texture.image,
                             static_cast<uint32_t>(texWidth),
                             static_cast<uint32_t>(texHeight),
                             context.device, context.transferQueue, context.transferCommandPool
    );

    utils::transitionImageLayout(texture.image,
                                 VK_FORMAT_R8G8B8A8_SRGB,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 context.device, context.graphicsQueue, context.graphicsCommandPool
    );


    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);


    utils::createImageView("texture", 
                           texture.image, 
                           VK_FORMAT_R8G8B8A8_SRGB,
                           VK_IMAGE_ASPECT_COLOR_BIT,
                           texture.imageView, context.device
    );

    return texture;
}
