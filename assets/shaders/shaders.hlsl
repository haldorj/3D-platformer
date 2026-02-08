struct Light
{
    float4 color;
    float4 dir;
    float4 ambient;
    float4 diffuse;
};

cbuffer cbPerFrame
{
    Light light;
};

cbuffer cbPerObject
{
    float4x4 GlobalBoneTransform[100];
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
    float4 Color;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    int4 boneIDs : BONEIDS;
    float4 weights : WEIGHTS;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD,
               int4 boneIDs : BONEIDS, float4 weights : WEIGHTS)
{
    PSInput result;

    float4 skinnedPosition = float4(0.0, 0.0, 0.0, 0.0);
    float4 skinnedNormal = float4(0.0, 0.0, 0.0, 0.0);

    for (int i = 0; i < 4; ++i)
    {
        int idx = clamp(boneIDs[i], 0, 100 - 1);
        skinnedPosition += mul(GlobalBoneTransform[idx], float4(position.xyz, 1.0f)) * weights[i];
        skinnedNormal += mul(GlobalBoneTransform[idx], float4(normal.xyz, 0.0f)) * weights[i];
    }

    float4 worldPos = mul(World, position);
    result.position = mul(Projection, mul(View, worldPos));

    // Correct normal computation
    float3 normal3 = mul((float3x3) World, normal.xyz);
    normal3 = normalize(normal3);
    result.normal = float4(normal3, 1.0f);

    result.texCoord = texCoord;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 objectColor = g_texture.Sample(g_sampler, input.texCoord);
    clip(objectColor.a - 0.1);

    float4 ambient = light.ambient * light.color;
    float4 diffuse = max(dot(input.normal.xyz, light.dir.xyz), 0.0f) * light.color;
    
    //return input.normal;
    return (ambient + diffuse) * objectColor;
}
