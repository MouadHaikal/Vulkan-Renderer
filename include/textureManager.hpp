#pragma once

#include <utilities.hpp>



class TextureManager{
public:
    static void setContext(const VulkanContext& ctx);


    struct Texture{
        VkImage        image;
        VkDeviceMemory imageMemory;
        VkImageView    imageView;
        VkSampler      sampler;
    };


    // RGBA8888
    void addTexture(TextureType type, const std::string& name);
    void addEmbeddedTexture(TextureType type, const std::string& name ,void* pixels, int width, int height);

    int         getTextureIndex(TextureType type, const std::string& name) const;
    size_t      getTextureCount(TextureType type) const;

    VkImageView getTextureImageView(TextureType type, size_t index) const;
    VkSampler   getTextureSampler(TextureType type, size_t index) const;


    void cleanup();

private:
    static VulkanContext context;


    std::unordered_map<TextureType, std::vector<Texture>>                 textures;
    std::unordered_map<TextureType, std::unordered_map<std::string, int>> textureIndices;


    Texture createTexture(void* pixels, int width, int height, VkFormat format);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void createTextureSampler(uint32_t mipLevels, VkSampler& textureSampler);
};
