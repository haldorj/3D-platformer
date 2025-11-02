
cbuffer cbPerObject
{
    float4x4 WVP;
    float4 Color;
};

struct PSInput
{
    float2 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float2 position : POSITION, float2 texCoord : TEXCOORD)
{
    PSInput result;

    result.position = mul(float4(position, 0.0f, 1.0f), WVP);
    result.texCoord = texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 diffuse = g_texture.Sample(g_sampler, input.texCoord);
    
    clip(diffuse.a - 0.1);
    
    //return input.normal;
    return diffuse * Color;
}
