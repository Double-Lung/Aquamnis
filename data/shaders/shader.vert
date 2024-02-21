#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;    
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 fragNormalWorld;

struct PointLight
{
    vec4 color;
    vec4 position;
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

void main() 
{
    vec4 worldPosition = entityUBO.transform * vec4(inPosition, 1.0);
    gl_Position = globalUBO.proj * globalUBO.view * worldPosition;

    fragNormalWorld = normalize(mat3(entityUBO.normalMat) * inNormal);
    fragPosWorld = worldPosition.xyz;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}