#version 460
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


layout(set = 1, binding = 0) uniform sampler2D combinedTextureSamplers[];


layout(push_constant) uniform FragmentPushConstant {
    layout(offset = 64) int  textureIndex;
    layout(offset = 80) vec3 albedoColor;
} fpc;


void main(){
    if (fpc.textureIndex != -1){
        outColor = texture(combinedTextureSamplers[nonuniformEXT(fpc.textureIndex)], fragTexCoord);
    }
    else{
        outColor = vec4(fpc.albedoColor, 1.0f);
    }
}
