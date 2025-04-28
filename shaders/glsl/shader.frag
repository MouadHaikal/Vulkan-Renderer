#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


layout(binding = 1) uniform sampler   texSampler;
layout(binding = 2) uniform texture2D textures[];


layout(push_constant) uniform FragmentPushConstant {
    layout(offset = 64) int  textureIndex;
    layout(offset = 80) vec3 albedoColor;
} fpc;


void main(){
    if(fpc.textureIndex != -1){
        outColor = texture(sampler2D(textures[nonuniformEXT(fpc.textureIndex)], texSampler), fragTexCoord);
    }
    else{
        outColor = vec4(fpc.albedoColor, 1.0f);
    }
}
