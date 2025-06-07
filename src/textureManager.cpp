#include <textureManager.hpp>

#include <logger.hpp>


VulkanContext TextureManager::context;


void TextureManager::setContext(const VulkanContext &ctx){ context = ctx; }


void TextureManager::addTexture(const std::string& name){
    if (textureIndices.find(name) != textureIndices.end()) {
        LOG_WARNING_S("Texture '" << name << "' already loaded");
        return;
    }


    std::string path = (dirTextures/name).string();

    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        LOG_WARNING("Failed to load texture image (" + path + ")");
        return;
    } 


    textureIndices[name] = static_cast<int>(textures.size());
    textures.push_back(createTexture(pixels, texWidth, texHeight));

    LOG_DEBUG("Texture loaded (" + name + ")");
}

void TextureManager::addEmbeddedTexture(const std::string& name ,void* pixels, int width, int height){
    textureIndices[name] = static_cast<int>(textures.size());
    textures.push_back(createTexture(pixels, width, height));

    LOG_DEBUG("Embedded texture loaded (" + name + ")");
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

VkImageView TextureManager::getTextureImageView(size_t index) const{ return textures[index].imageView; }

VkSampler TextureManager::getTextureSampler(size_t index) const{ return textures[index].sampler; }


void TextureManager::cleanup(){
    for (auto& texture : textures) {
        vkDestroyImageView(context.device, texture.imageView, nullptr);
        vkDestroyImage(context.device, texture.image, nullptr);
        vkFreeMemory(context.device, texture.imageMemory, nullptr);
        vkDestroySampler(context.device, texture.sampler, nullptr);
    }
}


TextureManager::Texture TextureManager::createTexture(void* pixels, int width, int height){
    Texture texture;


    VkDeviceSize imageSize = width * height * 4;

    uint32_t mipLevels = 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))));


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
                       width, height, 
                       mipLevels, VK_SAMPLE_COUNT_1_BIT,
                       VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       texture.image, texture.imageMemory,
                       context.physicalDevice, context.device, context.queueFamilyIndices
    );


    utils::transitionImageLayout(texture.image, 
                                 VK_FORMAT_R8G8B8A8_SRGB, mipLevels,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 context.device, context.graphicsQueue, context.graphicsCommandPool
    );

    utils::copyBufferToImage(stagingBuffer, texture.image,
                             static_cast<uint32_t>(width),
                             static_cast<uint32_t>(height),
                             context.device, context.transferQueue, context.transferCommandPool
    );


    generateMipmaps(texture.image, VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);


    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);


    utils::createImageView("texture", 
                           texture.image, 
                           VK_FORMAT_R8G8B8A8_SRGB,
                           mipLevels, VK_IMAGE_ASPECT_COLOR_BIT,
                           texture.imageView, context.device
    );


    createTextureSampler(mipLevels, texture.sampler);

    return texture;
}

void TextureManager::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels){
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(context.physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        LOG_ERROR("Texture image format does not support linear blitting");
    }

    VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(context.graphicsCommandPool, context.device);
        
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image                           = image;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.subresourceRange.levelCount     = 1;
        

        int32_t mipWidth  = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; ++i) {
            barrier.subresourceRange.baseMipLevel = i - 1;

            barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, 
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 
                                 0, nullptr, 
                                 0, nullptr, 
                                 1, &barrier
            );


            VkImageBlit blit{};
            blit.srcOffsets[0]                 = { 0, 0, 0 };
            blit.srcOffsets[1]                 = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel       = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = 1;

            blit.dstOffsets[0]                 = { 0, 0, 0 };
            blit.dstOffsets[1]                 = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = 1;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                           1, &blit, 
                           VK_FILTER_LINEAR
            );


            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, 
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                                 0, 
                                 0, nullptr, 
                                 0, nullptr, 
                                 1, &barrier
            );


            if (mipWidth  > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }


        barrier.subresourceRange.baseMipLevel = mipLevels - 1;

        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, 
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                             0,
                             0, nullptr, 
                             0, nullptr, 
                             1, &barrier
        );

    utils::endSingleTimeCommands(commandBuffer, context.device, context.graphicsCommandPool, context.graphicsQueue);
}

void TextureManager::createTextureSampler(uint32_t mipLevels, VkSampler& textureSampler){
    VkSamplerCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter        = VK_FILTER_LINEAR;
    createInfo.minFilter        = VK_FILTER_LINEAR;
    createInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(context.physicalDevice, &deviceProperties);

    createInfo.maxAnisotropy    = deviceProperties.limits.maxSamplerAnisotropy;

    createInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable           = VK_FALSE;
    createInfo.compareOp               = VK_COMPARE_OP_ALWAYS;

    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod     = 0.0f;
    createInfo.maxLod     = static_cast<float>(mipLevels);

    LOG_RESULT(
        vkCreateSampler(context.device, &createInfo, nullptr, &textureSampler), 
        "Create texture sampler"
    );
}

