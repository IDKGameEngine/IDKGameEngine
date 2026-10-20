#include "idk/slang.hpp"

layout(binding = 9) RWTexture2D<vec4> outputImage;
// layout(binding = 8) Texture2D<vec3> noiseImage;

SamplerState mySampler;

[shader("compute")]
[numthreads(8, 8, 1)]
void compmain(uvec3 threadId : SV_DispatchThreadID)
{
    uvec2 texel = uvec2(threadId.xy);
    vec2  uv = vec2(texel) / vec2(idk::AUTOMATA_WIDTH);

    // noiseImage.Sample(mySampler, uv);
    vec3 rgb = vec3(uv.y, uv.x*uv.y, 1.0f - uv.x);

    outputImage.Store(texel, vec4(rgb, 1.0f));
}
