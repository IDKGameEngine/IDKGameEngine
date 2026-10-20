#include "idk/slang.hpp"

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

    vec3  pos  = dataIn[i].pos.xyz;
    float mass = dataIn[i].pos.w;
    vec3  vel  = dataIn[i].vel.xyz;
    vec3  acc  = dataIn[i].acc.xyz;

    const float dt = frameData.deltaTime * frameData.timescale;
    vec3 newPos = pos + vel*dt + acc*(dt*dt*0.5);

    dataOut[i].pos = vec4(newPos, mass);
    dataOut[i].vel = dataIn[i].vel;
    dataOut[i].acc = dataIn[i].acc;
}

