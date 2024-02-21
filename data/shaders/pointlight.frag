#version 450

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

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
uniform EntityUBO
{
    mat4 transform;
    mat4 normalMat;
	vec4 color;
	float radius;
} entityUBO;

layout(set = 1, binding = 1) 
uniform sampler2D texSampler;

void main() {
    float toggle = step(sqrt(dot(fragOffset, fragOffset)), 1.0);
    outColor = vec4(entityUBO.color.xyz * toggle, toggle);
}