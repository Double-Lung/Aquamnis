#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;    
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 fragNormalWorld;

layout(push_constant)
uniform Push
{
    mat4 normalMat;
    mat4 transform;
} push;

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
    vec4 worldPosition = push.transform * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPosition;

    fragNormalWorld = normalize(mat3(push.normalMat) * inNormal);
    fragPosWorld = worldPosition.xyz;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}