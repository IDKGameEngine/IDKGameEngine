#include "idk/slang.hpp"

layout(binding = 8) RWTexture2D<vec4> inputImage;
layout(binding = 9) RWTexture2D<vec4> outputImage;


ivec3 countNeighbours(ivec2 texel)
{
    ivec3 count = ivec3(0, 0, 0);

    count += ivec3(255.0f * inputImage.Load(texel + ivec2(-1, -1)).xyz);
    count += ivec3(255.0f * inputImage.Load(texel + ivec2(-1,  0)).xyz);
    count += ivec3(255.0f * inputImage.Load(texel + ivec2(-1, +1)).xyz);
    count += ivec3(255.0f * inputImage.Load(texel + ivec2( 0, -1)).xyz);
    count += ivec3(255.0f * inputImage.Load(texel + ivec2( 0, +1)).xyz);
    count += ivec3(255.0f * inputImage.Load(texel + ivec2(+1, -1)).xyz);
    count += ivec3(255.0f * inputImage.Load(texel + ivec2(+1,  0)).xyz);
    count += ivec3(255.0f * inputImage.Load(texel + ivec2(+1, +1)).xyz);

    return (count / 255);
}


[shader("compute")]
[numthreads(8, 8, 1)]
void compmain(uvec3 threadId : SV_DispatchThreadID)
{
    ivec2 texel = ivec2(threadId.xy);

    ivec3 curr  = ivec3(inputImage.Load(texel).rgb);
    ivec3 frens = countNeighbours(texel);
    ivec3 value = ivec3(0, 0, 0);

    if (curr.r==0)
    {
        if (frens.r==3) { value.r = 1; }
    }
    else if (frens.r==2 || frens.r==3)
    {
        value.r = 1;
    }

    // if (curr.g==0)
    // {
    //     if (frens.g==3) { value.g = 1; }
    // }
    // else if (frens.g==2 || frens.g==3)
    // {
    //     value.g = 1;
    // }

    // if (curr.b==0)
    // {
    //     if (frens.b==3) { value.b = 1; }
    // }
    // else if (frens.b==2 || frens.b==3)
    // {
    //     value.b = 1;
    // }

    outputImage.Store(texel, vec4(value.r, value.r, value.r, 1));
}
