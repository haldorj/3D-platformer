
cbuffer cbPerObject
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
    float4 Color;
};

struct VSInput
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput result;

    result.position = mul(Projection, mul(World, float4(input.position.xy, 0.0f, 1.0f)));
    result.texCoord = input.texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 diffuse = g_texture.Sample(g_sampler, input.texCoord);

    clip(diffuse.a - 0.1);

    return diffuse * Color;
}
