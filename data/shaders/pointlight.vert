#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec2 fragOffset;

layout(set = 0, binding = 0) 
uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
    vec4 ambientColor;
    vec4 lightColor;
    vec3 lightPosition;
} ubo;

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, 1.0)
);

const float LIGHT_RADIUS = 0.1;

void main() 
{
    fragOffset = OFFSETS[gl_VertexIndex];
    gl_Position = ubo.proj * ubo.view * vec4(ubo.lightPosition, 1.0);
    gl_Position /= gl_Position.w;
    gl_Position.xy += LIGHT_RADIUS * vec2(3.0/4.0, 1.0)  * fragOffset;
}