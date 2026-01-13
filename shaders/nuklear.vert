#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

layout(binding = 1) uniform Uniforms {
    mat4 projection;
} uniforms;

void main() {
    gl_Position = uniforms.projection * vec4(position, 0.0, 1.0);
    out_uv = uv;
    out_color = color / 255.0;
}
