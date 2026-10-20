#ifndef TYPES_SLANG_H
#define TYPES_SLANG_H

typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;

typedef int2 ivec2;
typedef int3 ivec3;
typedef int4 ivec4;

typedef uint2 uvec2;
typedef uint3 uvec3;
typedef uint4 uvec4;

typedef float3x3 mat3;
typedef float4x4 mat4;

#define UniformBufferRO(T) [[vk::binding(T::BIND_IDX, 0)]] ConstantBuffer<T, Std140DataLayout>
#define StorageBufferRO(T) [[vk::binding(T::BIND_IDX, 0)]] StructuredBuffer<T, Std430DataLayout>
#define StorageBufferRW(T) [[vk::binding(T::BIND_IDX, 0)]] RWStructuredBuffer<T, Std430DataLayout>

#endif // TYPES_SLANG_H
