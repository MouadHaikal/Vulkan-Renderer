#include <logger.hpp>
#include <material.hpp>


Material::Material(glm::vec3 albedoColor) : albedoColor(albedoColor){
    for (int type = TEX_TYPE_BASE; type != TEX_TYPE_ENUM_SIZE; ++type) {
        textureIndices[static_cast<TextureType>(type)] = -1;
    }
}


void Material::setTextureIndex(TextureType type, int index){
    if (index < -1) {
        LOG_WARNING("Invalid texture index");
        return;
    }
    
    textureIndices[type] = index;
}

void Material::pushInfo(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const glm::vec3& cameraPos) const{
    FragmentPushConstant fragmentPC;
    fragmentPC.albedoColor      = albedoColor;
    fragmentPC.baseTextureIndex = textureIndices.at(TEX_TYPE_BASE);
    fragmentPC.cameraPos        = cameraPos;
    fragmentPC.normalMapIndex   = textureIndices.at(TEX_TYPE_NORMAL);

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VertexPushConstant), sizeof(FragmentPushConstant), &fragmentPC);
}
