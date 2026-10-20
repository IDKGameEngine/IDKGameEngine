#ifndef IDK_PROCEDURAL_SLANG
#define IDK_PROCEDURAL_SLANG

// Eight squares per world unit. Project onto the dominant face plane so all
// sides of a box retain their density, including baked level geometry.
// Integrating over the pixel footprint filters distant checks.
float checkerShade(float3 worldPosition)
{
    float3 position = worldPosition * 8.0;
    float3 width = max(fwidth(position), float3(0.0001));
    float3 lo = position - 0.5 * width;
    float3 hi = position + 0.5 * width;
    float3 average = (abs(2.0 * frac(lo * 0.5) - 1.0) -
                      abs(2.0 * frac(hi * 0.5) - 1.0)) / width;
    float3 normal = abs(cross(ddx(worldPosition), ddy(worldPosition)));
    float checks = normal.x >= normal.y && normal.x >= normal.z ? average.y * average.z
                   : normal.y >= normal.z ? average.x * average.z : average.x * average.y;
    return lerp(0.22, 1.0, saturate(0.5 + 0.5 * checks));
}

#endif
