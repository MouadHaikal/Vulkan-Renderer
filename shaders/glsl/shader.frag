#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 1) uniform sampler   texSampler;
layout(binding = 2) uniform texture2D textures[];

layout(push_constant) uniform PushConstants {
    int textureIndex;
} pushConstants;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main(){
    if(pushConstants.textureIndex != -1){
        outColor = texture(sampler2D(textures[nonuniformEXT(pushConstants.textureIndex)], texSampler), fragTexCoord);
    }
    else{
        outColor = vec4(fragColor, 1.0f);
    }
}
