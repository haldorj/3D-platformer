
cbuffer cbPerObject
{
    float4x4 WVP;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD)
{
    PSInput result;
    
    result.position = mul(position, WVP);
    result.normal = normal;
    result.texCoord = texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 diffuse = g_texture.Sample(g_sampler, input.texCoord);
    
    clip(diffuse.a - 0.1);
    
    //return input.normal;
    return diffuse;
}
