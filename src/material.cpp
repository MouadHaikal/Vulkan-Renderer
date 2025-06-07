#include <logger.hpp>
#include <material.hpp>


Material::Material(int textureIndex, glm::vec3 albedoColor) : textureIndex(textureIndex), albedoColor(albedoColor){}


void Material::setTextureIndex(int index){
    if (index < -1) {
        LOG_WARNING("Invalid texture index");
        return;
    }
    textureIndex = index;
}

void Material::pushInfo(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const{
    FragmentPushConstant fragmentPC;
    fragmentPC.textureIndex = textureIndex;
    fragmentPC.albedoColor  = albedoColor;

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VertexPushConstant), sizeof(FragmentPushConstant), &fragmentPC);
}
