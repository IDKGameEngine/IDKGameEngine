#include "idk/slang.hpp"

#define M_PI 3.14159265359


[[vk::binding(idk::PerFrameData::BIND_IDX, 0)]]
ConstantBuffer<idk::PerFrameData, Std140DataLayout> UboWindow_;

[[vk::binding(idk::PerCameraData::BIND_IDX, 0)]]
ConstantBuffer<idk::PerCameraData, Std140DataLayout> uboCamera_;

[[vk::binding(idk::BIND_NBODY_IN, 0)]]
StructuredBuffer<idk::NBodyVertex, Std430DataLayout> vertIn;

[[vk::binding(idk::BIND_NBODY_OUT, 0)]]
StructuredBuffer<idk::NBodyVertex, Std430DataLayout> vertOut;

struct VertexOutput
{
    uint    vertId;
    float4  worldpos;
    float4  sv_position  : SV_Position;
    float   pointSize;    // : TEXCOORD1;
    float2  screenCenter; // : TEXCOORD0;   // point centre in pixels
};


[shader("vertex")]
VertexOutput vertmain(uint vertId: SV_VertexID, out float sv_pointsize: SV_PointSize)
{
    VertexOutput vsout;

    float4 wpos = float4(vertIn[vertId].pos.xyz, 1.0);
    float  mass = vertIn[vertId].pos.w;
    float  rad3 = (3*mass) / (4.0*M_PI);
    float  rad  = pow(rad3, 1.0/3.0);
           rad *= 512.0;

    vsout.vertId      = vertId;
    vsout.worldpos    = wpos;
    vsout.sv_position = mul(uboCamera_.Proj, mul(uboCamera_.View, wpos));
    vsout.pointSize   = max(2.0, rad / vsout.sv_position.w);

    float2 ndc = vsout.sv_position.xy / vsout.sv_position.w;
    vsout.screenCenter = float2(
        (ndc.x *  0.5 + 0.5) * UboWindow_.winSize.x,
        (ndc.y * -0.5 + 0.5) * UboWindow_.winSize.y  // <-- negated
    );
    sv_pointsize = vsout.pointSize;

    return vsout;
}


[shader("fragment")]
float4 fragmain(VertexOutput fsin) : SV_Target0
{
    float2 offset = (fsin.sv_position.xy - fsin.screenCenter) / fsin.pointSize;
    if (dot(offset, offset) > 0.25)
    {
        discard;
    }

    float minMass = idk::NBodyVertex::MIN_MASS;
    float maxMass = idk::NBodyVertex::MAX_MASS;

    float mass    = vertIn[fsin.vertId].pos.w;
    float mass4   = mass*mass*mass*mass;

    float alpha   = (mass / maxMass);
    // float alpha4  = clamp(mass4, 0.0, maxMass) / maxMass;

    // vec3 rgb = vec3(alpha, 0.75*alpha, 0.5);
    // vec3 rgb = vec3(alpha);
    vec3 rgb = 2.5*vec3(0.22f, 0.13f, 0.37f);

    return vec4(rgb, 0.95);
}