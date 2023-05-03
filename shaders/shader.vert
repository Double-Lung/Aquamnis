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
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, 1.0, 1.0));
const float AMBIENT = 0.05;

void main() {
    gl_Position = ubo.proj * ubo.view * push.transform * vec4(inPosition, 1.0);
    vec3 normal = normalize(mat3(push.normalMat) * inNormal);
    float lightIntensity = max(dot(normal, DIRECTION_TO_LIGHT), AMBIENT);

    fragColor = inColor * lightIntensity;
    fragTexCoord = inTexCoord;
}