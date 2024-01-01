#version 450

layout(location = 0) out vec2 fragOffset;

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

layout(push_constant) uniform Push
{
    vec4 position;
    vec4 color;
    float radius;
} push;

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, 1.0)
);

void main() 
{
    fragOffset = OFFSETS[gl_VertexIndex];
    gl_Position = ubo.proj * ubo.view * push.position;
    gl_Position /= gl_Position.w;
    gl_Position.xy += push.radius * vec2(3.0/4.0, 1.0)  * fragOffset;
}