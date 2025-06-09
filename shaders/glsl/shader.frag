#version 460
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;


struct PointLight{
    vec3  position;
    float intensity;
    vec3  color;
};

struct DirectionalLight{
    vec3  direction;
    float normalIrradiance;
    vec3  color;
};

layout(set = 0, binding = 1) uniform PointLightData{
    int        lightCount;
    PointLight lights[];
} pointLightData;

layout(set = 0, binding = 2) uniform DirectionalLightData{
    int              lightCount;
    DirectionalLight lights[];
} directionalLightData;


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
        outColor = vec4(fpc.albedoColor, 1.0);
    }

    // outColor = vec4((fragNormal.x + 1) / 2, (fragNormal.y + 1) / 2, (fragNormal.z + 1) / 2, 1.0);
}
