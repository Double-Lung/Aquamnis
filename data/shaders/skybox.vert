#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;    
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outUVW;

struct PointLight
{
    vec4 color;
    vec3 position;
};

layout(set = 0, binding = 0) 
uniform GlobalUBO
{
    PointLight pointLights[10];
    mat4 view;
    mat4 invView;
    mat4 proj;
    vec4 ambientColor;
    vec3 directLightDirection;
    int numLights;
} globalUBO;

void main() 
{
	outUVW = inPos;

	// Remove translation from view matrix
	mat4 viewMat = mat4(mat3(globalUBO.view));
	gl_Position = globalUBO.proj * viewMat * vec4(inPos.xyz, 1.0);
}