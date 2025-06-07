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
    void addTexture(const std::string& name);
    void addEmbeddedTexture(const std::string& name ,void* pixels, int width, int height);

    int         getTextureIndex(const std::string& name) const;
    size_t      getTextureCount() const;

    VkImageView getTextureImageView(size_t index) const;
    VkSampler   getTextureSampler(size_t index) const;


    void cleanup();

private:
    static VulkanContext context;


    std::vector<Texture> textures;

    std::unordered_map<std::string, int> textureIndices = {{"", -1}};


    Texture createTexture(void* pixels, int width, int height);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void createTextureSampler(uint32_t mipLevels, VkSampler& textureSampler);
};
