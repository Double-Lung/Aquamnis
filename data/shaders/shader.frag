#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragNormalWorld;

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
    vec4 ambientColor;
    PointLight pointLights[2];
    int numLights;
} ubo;

layout(binding = 1) 
uniform sampler2D texSampler;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, 1.0, 1.0));

void main() {
    vec3 diffuseLight = ubo.ambientColor.xyz * ubo.ambientColor.w;
    vec3 surfaceNormal = normalize(fragNormalWorld);
    for (int i = 0; i < ubo.numLights; ++i)
    {
        PointLight light = ubo.pointLights[i];
        vec3 dirToPointLight = light.position.xyz - fragPosWorld;
        float ligthFallOff = 1.0 / dot(dirToPointLight, dirToPointLight);
        float cosAngIncidence = max(dot(surfaceNormal, normalize(dirToPointLight)), 0);
        vec3 singleLightColor = light.color.xyz * light.color.w * ligthFallOff;
        diffuseLight += singleLightColor *  cosAngIncidence;
    }

    float directLight = max(dot(surfaceNormal, DIRECTION_TO_LIGHT), 0.0);
    outColor = vec4(fragColor * (diffuseLight + directLight), 1.0); // texture(texSampler, fragTexCoord) * 
}