#pragma once

#include <utilities.hpp>



class TextureManager{
public:
    static void setContext(const VulkanContext& ctx);


    struct Texture{
        VkImage        image       = VK_NULL_HANDLE;
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
        VkImageView    imageView   = VK_NULL_HANDLE;
    };

    void addTexture(const std::string& name, const std::string& path);

    int         getTextureIndex(const std::string& name) const;
    size_t      getTextureCount() const;
    VkImageView getTextureImageView(size_t index) const;


    void cleanup();

private:
    static VulkanContext context;


    std::vector<Texture> textures;

    std::unordered_map<std::string, int> textureIndices = {{"None", -1}};


    Texture createTexture(const std::string& imagePath);
};
