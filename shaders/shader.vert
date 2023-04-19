#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(push_constant)
uniform Push
{
    vec3 offset;
    mat4 transform;
} push;

layout(set = 0, binding = 0) 
uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * push.transform * vec4(inPosition + push.offset, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}