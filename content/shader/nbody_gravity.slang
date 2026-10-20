#include "idk/slang.hpp"

#define softening_ 0.01f // prevents singularity when distance → 0
#define num_bodies_ idk::NBodyVertex::MAX_BODIES

[[vk::binding(idk::PerFrameData::BIND_IDX, 0)]]
ConstantBuffer<idk::PerFrameData, Std140DataLayout> frameData;

[[vk::binding(idk::BIND_NBODY_IN, 0)]]
StructuredBuffer<idk::NBodyVertex, Std430DataLayout> dataIn;

[[vk::binding(idk::BIND_NBODY_OUT, 0)]]
RWStructuredBuffer<idk::NBodyVertex, Std430DataLayout> dataOut;


[shader("compute")]
[numthreads(idk::NBodyVertex::GROUP_SIZE, 1, 1)]
void compmain(uvec3 threadId : SV_DispatchThreadID)
{
    uint i = threadId.x;
    if (i >= num_bodies_)
    {
        return;
    }

    const float dt = frameData.deltaTime * frameData.timescale;

    vec3  pos_i  = dataIn[i].pos.xyz;
    float mass_i = dataIn[i].pos.w;
    vec3  vel_i  = dataIn[i].vel.xyz;
    vec3  acc_i  = dataIn[i].acc.xyz;

    vec3 newAcc = vec3(0.0);
    for (uint j=0; j<num_bodies_; j++)
    {
        if (i == j)
        {
            continue;
        }

        vec3  diff     = dataIn[j].pos.xyz - pos_i;
        float distSq   = dot(diff, diff) + softening_ * softening_;
        float invDist3 = rsqrt(distSq * distSq * distSq);
        float mass_j   = dataIn[j].pos.w;

        newAcc += mass_j * diff * invDist3;
    }

    vec3 newVel = vel_i + 0.5f * (acc_i + newAcc) * dt;

    dataOut[i].pos = dataIn[i].pos;
    dataOut[i].vel = vec4(newVel, 0.0);
    dataOut[i].acc = vec4(newAcc, 0.0);
}

