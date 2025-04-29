#include <scene.hpp>


VulkanContext Scene::context;


void Scene::setContext(const VulkanContext& ctx){ 
    context = ctx;
    TextureManager::setContext(ctx);
    Mesh::setContext(ctx);
}


void Scene::addTexture(const std::string& name, const std::string& path){
    textureManager.addTexture(name, path);
}

glm::mat4* Scene::addObject(std::variant<std::string, RawMesh> source, const std::string& textureName, glm::vec3 albedoColor){
    objects.emplace_back(source, textureManager.getTextureIndex(textureName), albedoColor);
    return objects.back().getModelMatrix();
}


size_t Scene::getTextureCount() const{ return textureManager.getTextureCount(); }

VkImageView Scene::getTextureImageView(size_t index) const{ return  textureManager.getTextureImageView(index); }


void Scene::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const{
    for (const auto& object : objects) {
        object.draw(commandBuffer, pipelineLayout);
    }
}


void Scene::cleanup(){
    textureManager.cleanup();

    for (auto& object : objects) {
        object.cleanup();
    }
}
