#version 460
#extension GL_ARB_separate_shader_objects : enable

const float PI = 3.1415926535897932384626433832795;
const float PI_2 = 1.57079632679489661923;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 midpoint;

layout(location = 0) out vec3 fragColor;

void main() {
    const float MAX_LIGHT_DISTANCE = 50.0;
    float d = distance(midpoint, vec3(0.0, 0.0, 0.0));
    float i = clamp(d, 0, MAX_LIGHT_DISTANCE) / MAX_LIGHT_DISTANCE;
    fragColor = (1 - i) * vec3(1.0, 0.0, 0.0);

    gl_Position = ubo.proj * ubo.view * vec4(pos, 1.0);
}
