#version 460


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;


layout(push_constant) uniform VertexPushConstant{
    mat4 model;
} vpc;


void main() {
    vec4 worldPos = vpc.model * vec4(inPosition, 1.0);

    gl_Position  = ubo.proj * ubo.view * worldPos;

    fragPosition = worldPos.xyz;
    fragTexCoord = inTexCoord;
    fragNormal   = inNormal;
}
