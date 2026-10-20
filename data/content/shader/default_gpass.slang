struct VertexShaderInput
{
    float3 pos;
    float3 nrm;
    float2 tex;
};

struct VertexShaderOutput
{
    float3 wpos;
    float3 wnrm;
    float2 tex;
    float4 sv_position: SV_Position;
};

struct FragmentShaderOutput
{
    float4 color;
};

struct camera_t
{
    float4x4 M;
    float4x4 V;
    float4x4 P;
    float4x4 MVP;
};

cbuffer Uniforms
{
    camera_t cam;
};


[shader("vertex")]
VertexShaderOutput vertmain(VertexShaderInput vsin)
{
    VertexShaderOutput vsout;

    float3 position = mul(cam.M, float4(vsin.pos, 1.0)).xyz;

    vsout.wpos = position;
    vsout.wnrm = normalize(mul(float3x3(cam.M), vsin.nrm.xyz));
    vsout.tex  = vsin.tex;
    vsout.sv_position = mul(cam.P * cam.V, float4(position, 1.0));

    return vsout;
}


[shader("fragment")]
FragmentShaderOutput fragmain(VertexShaderOutput fsin): SV_Target
{
    FragmentShaderOutput fsout;

    fsout.color = float4(fsin.wnrm, 1.0);

    return fsout;
}

