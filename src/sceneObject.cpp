#include <sceneObject.hpp>


SceneObject::SceneObject(std::variant<std::string, RawMesh> source, int textureIndex, glm::vec3 albedoColor) : 
    mesh(source), material(textureIndex, albedoColor){}


void SceneObject::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const{
    mesh.pushInfo(commandBuffer, pipelineLayout);

    material.pushInfo(commandBuffer, pipelineLayout);

    mesh.draw(commandBuffer);
}


void SceneObject::cleanup(){
    mesh.cleanup();
}
