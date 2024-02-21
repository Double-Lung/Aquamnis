#version 450

layout(location = 0) in vec3 inUVW;
layout(location = 0) out vec4 outFragColor;

struct PointLight
{
    vec4 color;
    vec3 position;
};

layout(set = 0, binding = 0) 
uniform GlobalUBO
{
    PointLight pointLights[8];
    mat4 view;
    mat4 proj;
    mat4 invView;
    vec4 ambientColor;
    vec3 directLightDirection;
    int numLights;
} globalUBO;

layout(set = 1, binding = 0) 
uniform samplerCube samplerCubeMap;

void main() 
{
	outFragColor = texture(samplerCubeMap, inUVW);
}