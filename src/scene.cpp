#include <scene.hpp>


VulkanContext Scene::context;


void Scene::setContext(const VulkanContext& ctx){ 
    context = ctx;
    TextureManager::setContext(ctx);
    Mesh::setContext(ctx);
}


std::shared_ptr<Model> Scene::addModel(std::variant<std::string, RawMesh*> source){
    models.emplace_back(std::make_shared<Model>(source, this));
    return models.back();
}


void Scene::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const glm::vec3& cameraPos) const{
    for (const auto& model : models) {
        model->draw(commandBuffer, pipelineLayout, cameraPos);
    }
}


void Scene::cleanup(){
    textureManager.cleanup();

    for (auto& model : models) {
        model->cleanup();
    }
}
