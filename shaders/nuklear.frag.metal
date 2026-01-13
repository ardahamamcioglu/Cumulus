#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

fragment float4 main0(VertexOut in [[stage_in]], texture2d<float> texture [[texture(0)]], sampler samplr [[sampler(0)]]) {
    return in.color * texture.sample(samplr, in.uv);
}
