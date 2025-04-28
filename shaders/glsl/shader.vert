#version 450


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;


layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;


layout(push_constant) uniform VertexPushConstant{
    mat4 model;
} vpc;


void main() {
    gl_Position  = ubo.proj * ubo.view * vpc.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
