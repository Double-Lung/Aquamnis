#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;    
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outUVW;

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

void main() 
{
	outUVW = inPos;

	// Remove translation from view matrix
	mat4 viewMat = mat4(mat3(ubo.view));
	gl_Position = ubo.proj * viewMat * vec4(inPos.xyz, 1.0);
}