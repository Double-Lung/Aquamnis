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
    mat4 invView;
    vec4 ambientColor;
    PointLight pointLights[8];
    int numLights;
} ubo;

layout(binding = 1) 
uniform sampler2D texSampler;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, 1.0, 1.0));

void main() {
    vec3 diffuseLight = ubo.ambientColor.xyz * ubo.ambientColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDir = normalize(cameraPosWorld - fragPosWorld);

    // global specular Light
    // vec3 halfAngleVecG = normalize(DIRECTION_TO_LIGHT + viewDir);
    // float blinnTermG = dot(surfaceNormal, halfAngleVecG);
    // blinnTermG = clamp(blinnTermG, 0, 1);
    // blinnTermG = pow(blinnTermG, 32.0);
    // specularLight += vec3(1.f, 1.f, 1.f) * blinnTermG;

    for (int i = 0; i < ubo.numLights; ++i)
    {
        PointLight light = ubo.pointLights[i];
        vec3 dirToPointLight = light.position.xyz - fragPosWorld;
        float ligthFallOff = 1.0 / dot(dirToPointLight, dirToPointLight);
        dirToPointLight = normalize(dirToPointLight);
        float cosAngIncidence = max(dot(surfaceNormal, dirToPointLight), 0);
        vec3 singleLightColor = light.color.xyz * light.color.w * ligthFallOff;
        diffuseLight += singleLightColor *  cosAngIncidence;

        // specularLight
        vec3 halfAngleVec = normalize(dirToPointLight + viewDir);
        float blinnTerm = dot(surfaceNormal, halfAngleVec);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 32.0);
        specularLight += singleLightColor * blinnTerm;
    }

    float directLight = max(dot(surfaceNormal, DIRECTION_TO_LIGHT), 0.0);
    outColor = vec4(fragColor * (diffuseLight + specularLight), 1.0); // texture(texSampler, fragTexCoord) * 
}