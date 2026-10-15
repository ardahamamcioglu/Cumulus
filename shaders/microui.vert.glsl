#version 450
/*
 * microui SDL3 GPU backend — vertex shader (Vulkan / SPIR-V).
 *
 * Mirrors the MSL shader embedded in src/microui_sdl3_gpu.h so that the
 * Metal and Vulkan paths produce identical output:
 *
 *   clip = projection * vec4(position, 0, 1)
 *   color = vertex_color / 255     (UBYTE4, non-normalized)
 *
 * Compiled to SPIR-V at build-time / offline; see shaders/compile_spirv.sh.
 */

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in uvec4 in_color; /* SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4 */

layout(binding = 0, std140) uniform Uniforms {
    mat4 projection;
} uniforms;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

void main()
{
    gl_Position = uniforms.projection * vec4(in_position, 0.0, 1.0);
    out_uv = in_uv;
    out_color = vec4(in_color) / 255.0;
}
