#version 460


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 tbn;


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;


layout(push_constant) uniform VertexPushConstant{
    mat4 model;
} vpc;


void main() {
    vec4 worldPos = vpc.model * vec4(inPosition, 1.0);

    fragPosition = worldPos.xyz;
    fragTexCoord = inTexCoord;

    tbn = mat3(
        normalize(vec3(vpc.model * vec4(inTangent, 0.0))),
        normalize(vec3(vpc.model * vec4(inBitangent, 0.0))),
        normalize(vec3(vpc.model * vec4(inNormal, 0.0)))
    );

    gl_Position  = ubo.proj * ubo.view * worldPos;
}
