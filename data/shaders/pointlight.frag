#version 450

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) 
uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
    vec4 ambientColor;
    vec4 lightColor;
    vec3 lightPosition;
} ubo;

void main() {
    float toggle = step(sqrt(dot(fragOffset, fragOffset)), 1.0);
    outColor = vec4(ubo.lightColor.xyz * toggle, 0.0);
}