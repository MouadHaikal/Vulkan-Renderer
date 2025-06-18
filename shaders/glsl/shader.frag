#version 460
#extension GL_EXT_nonuniform_qualifier : require

const float AMBIENT   = 0.01;
const float DIFFUSE   = 1.0;
const float SPECULAR  = 1.0;
const int   PHONG_EXP = 200;


layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 tbn;

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
    PointLight lights[1];         // Array size does not matter here
} plData;

layout(set = 0, binding = 2) uniform DirectionalLightData{
    int              lightCount;
    DirectionalLight lights[1];   // Array size does not matter here
} dlData;


layout(set = 1, binding = 0) uniform sampler2D baseTextureSamplers[];
layout(set = 1, binding = 1) uniform sampler2D normalTextureSamplers[];


layout(push_constant) uniform FragmentPushConstant {
    layout(offset = 64) vec3 albedoColor;
    layout(offset = 76) int  baseTextureIndex;
    layout(offset = 80) vec3 cameraPos;
    layout(offset = 92) int  normalMapIndex;
} fpc;



vec3 normal;    // World space normal
vec3 baseCol;   // Base color

vec3 evalPointLights();
vec3 evalDirectionalLights();


void main(){
    // Setting global variables
    if (fpc.normalMapIndex != -1) {
        vec3 mapNormal = texture(normalTextureSamplers[nonuniformEXT(fpc.normalMapIndex)], fragTexCoord).rgb;
        mapNormal = 2.0 * mapNormal - vec3(1.0);
        normal = normalize(tbn * mapNormal);
    } else {
        normal = tbn[2];  // Interpolated vertex normal
    }

    baseCol = fpc.baseTextureIndex != -1 ? 
        texture(baseTextureSamplers[nonuniformEXT(fpc.baseTextureIndex)], fragTexCoord).rgb :
        fpc.albedoColor;


    // Light calcultions
    vec3 fragCol = AMBIENT * baseCol;

    fragCol += evalPointLights();
    fragCol += evalDirectionalLights();

    outColor = vec4(fragCol, 1.0);
    // outColor = vec4(0.5 * (normal + vec3(1.0)), 1.0);
}


vec3 evalPointLights(){
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
                DIFFUSE * max(0.0, dot(normal, lightVec)) * baseCol +
                vec3(SPECULAR * pow(max(0.0, dot(normal, halfVec)), PHONG_EXP))
        );
    }

    return result;
}

vec3 evalDirectionalLights(){
    vec3 result = vec3(0.0);

    vec3 lightVec, viewVec, halfVec;

    for(int i = 0; i < dlData.lightCount; i++){
        lightVec = normalize(-dlData.lights[i].direction);
        viewVec  = normalize(fpc.cameraPos - fragPosition);
        halfVec  = normalize(lightVec + viewVec);

        result  += dlData.lights[i].normalIrradiance * dlData.lights[i].color * (
                DIFFUSE * max(0.0, dot(normal, lightVec)) * baseCol +
                vec3(SPECULAR * pow(max(0.0, dot(normal, halfVec)), PHONG_EXP))
        );
    }

    return result;
}
