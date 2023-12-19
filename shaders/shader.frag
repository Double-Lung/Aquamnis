#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragNormalWorld;

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

layout(binding = 1) 
uniform sampler2D texSampler;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, 1.0, 1.0));

void main() {
    vec3 dirToPointLight = ubo.lightPosition - fragPosWorld;
    float ligthFallOff = 1.0 / dot(dirToPointLight,dirToPointLight);
    vec3 pointLightColor = ubo.lightColor.xyz * ubo.lightColor.w * ligthFallOff;
    vec3 ambientLight = ubo.ambientColor.xyz * ubo.ambientColor.w;
    float directLight = max(dot(fragNormalWorld, DIRECTION_TO_LIGHT), 0.0);
    vec3 diffuseLight = pointLightColor * max(dot(normalize(fragNormalWorld), normalize(dirToPointLight)), 0) + ambientLight;

    outColor = vec4(fragColor * (diffuseLight + directLight), 1.0); // texture(texSampler, fragTexCoord) * 
}