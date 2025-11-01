
//cbuffer constantBuffer : register(b0)
//{
//    float4x4 model;
//    float4x4 projection;
//};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    //float2 texCoord : TEXCOORD;
};

//Texture2D g_texture : register(t0);
//SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL)//, float2 texCoord : TEXCOORD)
{
    PSInput result;
    
    result.position = position;
    result.normal = normal;
    // result.texCoord = texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //float4 diffuse = g_texture.Sample(g_sampler, input.texCoord);
    
    //clip(diffuse.a - 0.1); // Discard pixels with low alpha
    
    return input.normal;
    //return diffuse;
}
