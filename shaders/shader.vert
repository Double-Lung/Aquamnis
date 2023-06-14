#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(push_constant)
uniform Push
{
    mat4 normalMat;
    mat4 transform;
} push;

layout(set = 0, binding = 0) 
uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
    vec4 ambientColor;
    vec4 lightColor;
    vec3 lightPosition;
} ubo;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, 1.0, 1.0));

void main() 
{
    vec4 worldPosition = push.transform * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPosition;
    vec3 normal = normalize(mat3(push.normalMat) * inNormal);

    vec3 dirToPointLight = ubo.lightPosition - worldPosition.xyz;
    float ligthFallOff = 1.0 / dot(dirToPointLight,dirToPointLight);
    vec3 pointLightColor = ubo.lightColor.xyz * ubo.lightColor.w * ligthFallOff;
    vec3 ambientLight = ubo.ambientColor.xyz * ubo.ambientColor.w;
    float directLight = max(dot(normal, DIRECTION_TO_LIGHT), 0.0);
    vec3 diffuseLight = pointLightColor * max(dot(normal, normalize(dirToPointLight)), 0) + ambientLight;

    fragColor = diffuseLight + directLight;
    fragTexCoord = inTexCoord;
}