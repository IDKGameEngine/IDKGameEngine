#include "idk/gfx/slang/slang.hpp"
#include "idk/gfx/slang/procedural.slang"

UniformBufferRO(idk::PerCameraData) camera;
UniformBufferRO(idk::PerDrawData) draw;
UniformBufferRO(idk::MaterialData) material;

struct VertexInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(2)]] float3 normal : NORMAL;
    [[vk::location(4)]] float4 color : COLOR0;
};
struct VertexOutput
{
    float4 position : SV_Position;
    [[vk::location(0)]] float3 normal : NORMAL;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float3 worldPosition : TEXCOORD0;
};
[shader("vertex")]
VertexOutput vertmain(VertexInput input)
{
    VertexOutput output;
    output.color = input.color;
    float4 worldPosition = mul(draw.Model, float4(input.position, 1));
    output.worldPosition = worldPosition.xyz;
    output.position = mul(camera.Proj, mul(camera.View, worldPosition));
    float3 a = mul(draw.Model, float4(1, 0, 0, 0)).xyz;
    float3 b = mul(draw.Model, float4(0, 1, 0, 0)).xyz;
    float3 c = mul(draw.Model, float4(0, 0, 1, 0)).xyz;
    // Cofactors give the inverse transpose after normalization, including
    // nonuniform scales, without an inverse in every fragment.
    output.normal = normalize(input.normal.x * cross(b, c) + input.normal.y * cross(c, a) + input.normal.z * cross(a, b)) * sign(dot(a, cross(b, c)));
    return output;
}
[shader("fragment")]
float4 fragmain(VertexOutput input, bool front : SV_IsFrontFace) : SV_Target0
{
    float3 normal = normalize(input.normal) * (front ? 1 : -1);
    float lighting = .28 + .72 * max(dot(normal, normalize(float3(1, 2, 3))), 0);
    float4 color = material.baseColor * input.color;
    if (material.alpha.z == 1.0) color.rgb *= checkerShade(input.worldPosition);
    if (material.alpha.x == 1 && color.a < material.alpha.y) discard;
    return float4(color.rgb * lighting, material.alpha.x == 2 ? color.a : 1);
}
