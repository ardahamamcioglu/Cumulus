#version 450
/*
 * microui SDL3 GPU backend — fragment shader (Vulkan / SPIR-V).
 *
 * Mirrors the MSL shader embedded in src/microui_sdl3_gpu.h:
 *
 *   out_color = in_color * texture(tex, uv)
 */

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = in_color * texture(tex, in_uv);
}
