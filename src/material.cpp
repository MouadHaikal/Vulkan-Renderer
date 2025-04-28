#include <material.hpp>


Material::Material(int textureIndex, glm::vec3 albedoColor) : textureIndex(textureIndex), albedoColor(albedoColor){}


void Material::pushInfo(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const{
    FragmentPushConstant fragmentPC;
    fragmentPC.textureIndex = textureIndex;
    fragmentPC.albedoColor  = albedoColor;

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VertexPushConstant), sizeof(FragmentPushConstant), &fragmentPC);
}
