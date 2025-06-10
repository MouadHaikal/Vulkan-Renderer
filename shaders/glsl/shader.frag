#version 460
#extension GL_EXT_nonuniform_qualifier : require

const int   MAX_PL    = 10;   // Point lights
const int   MAX_DL    = 10;   // Directional lights

const float AMBIENT   = 0.01;
const float DIFFUSE   = 1.0;
const float SPECULAR  = 1.0;
const int   PHONG_EXP = 200;


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
    PointLight lights[MAX_PL];
} plData;

layout(set = 0, binding = 2) uniform DirectionalLightData{
    int              lightCount;
    DirectionalLight lights[MAX_DL];
} dlData;


layout(set = 1, binding = 0) uniform sampler2D combinedTextureSamplers[];


layout(push_constant) uniform FragmentPushConstant {
    layout(offset = 64) vec3 albedoColor;
    layout(offset = 76) int  textureIndex;
    layout(offset = 80) vec3 cameraPos;
} fpc;


vec3 evaluatePointLights(vec3 base);
vec3 evaluateDirectionalLights(vec3 base);

void main(){
    vec3 baseCol = fpc.textureIndex != -1 ? 
        texture(combinedTextureSamplers[nonuniformEXT(fpc.textureIndex)], fragTexCoord).rgb :
        fpc.albedoColor;

    vec3 fragCol = AMBIENT * baseCol;

    fragCol += evaluatePointLights(baseCol);
    fragCol += evaluateDirectionalLights(baseCol);

    outColor = vec4(fragCol, 1.0);

    // outColor = vec4((fragNormal.x + 1) / 2, (fragNormal.y + 1) / 2, (fragNormal.z + 1) / 2, 1.0);
}

vec3 evaluatePointLights(vec3 base){
    vec3 result = vec3(0.0);
    
    vec3  lightVec, viewVec, halfVec;
    float distance, attenuation;

    for(int i = 0; i < plData.lightCount; i++){
        lightVec    = plData.lights[i].position - fragPosition;
        distance    = length(lightVec);
        lightVec    = normalize(lightVec);

        viewVec     = normalize(fpc.cameraPos - fragPosition);
        halfVec     = normalize(lightVec + viewVec);

        attenuation = plData.lights[i].intensity / (distance * distance);

        result     += attenuation * plData.lights[i].color * (
                DIFFUSE * max(0.0, dot(fragNormal, lightVec)) * base +
                vec3(SPECULAR * pow(max(0.0, dot(fragNormal, halfVec)), PHONG_EXP))
        );
    }

    return result;
}

vec3 evaluateDirectionalLights(vec3 base){
    vec3 result = vec3(0.0);

    vec3 lightVec, viewVec, halfVec;

    for(int i = 0; i < dlData.lightCount; i++){
        lightVec = normalize(-dlData.lights[i].direction);
        viewVec  = normalize(fpc.cameraPos - fragPosition);
        halfVec  = normalize(lightVec + viewVec);

        result  += dlData.lights[i].normalIrradiance * dlData.lights[i].color * (
                DIFFUSE * max(0.0, dot(fragNormal, lightVec)) * base +
                vec3(SPECULAR * pow(max(0.0, dot(fragNormal, halfVec)), PHONG_EXP))
        );
    }

    return result;
}
