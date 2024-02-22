#version 450

layout(location = 0) out vec2 fragOffset;

struct PointLight
{
    vec4 color;
    vec3 position;
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
    gl_Position = globalUBO.proj * globalUBO.view * entityUBO.transform[3];
    gl_Position /= gl_Position.w;
    gl_Position.xy += entityUBO.radius * vec2(1.0, globalUBO.proj[1][1]/globalUBO.proj[0][0]) * fragOffset; 
}