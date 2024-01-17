#version 450

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

struct PointLight
{
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) 
uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
    mat4 invView;
    vec4 ambientColor;
    PointLight pointLights[8];
    int numLights;
    float deltaTime;
} ubo;

layout(push_constant) uniform Push
{
    vec4 position;
    vec4 color;
    float radius;
} push;

void main() {
    float toggle = step(sqrt(dot(fragOffset, fragOffset)), 1.0);
    outColor = vec4(push.color.xyz * toggle, toggle);
}