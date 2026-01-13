#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
    uchar4 color [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

struct Uniforms {
    float4x4 projection;
};

vertex VertexOut main0(VertexIn in [[stage_in]], constant Uniforms &uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position = uniforms.projection * float4(in.position, 0.0, 1.0);
    out.uv = in.uv;
    out.color = float4(in.color) / 255.0;
    return out;
}
