#include "LightHelper.fx"

cbuffer cbPerFrame
{
    DirectionalLight gDirLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    float3 gEyePosW;
};

cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gWorldViewProj;
    Material gMaterial;
};


// Vertex Shader
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};


VertexOut VS(VertexIn inputVS)
{
    VertexOut outputVS;
    
    outputVS.PosW = mul(float4(inputVS.PosL, 1.0f), gWorld).xyz;
    outputVS.NormalW = mul(inputVS.NormalL, (float3x3) gWorldInvTranspose);
    
    outputVS.PosH = mul(float4(inputVS.PosL, 1.0f), gWorldViewProj);
    
    return outputVS;
}

// Pixel Shader
float4 PS(VertexOut inputVS) : SV_Target
{
    inputVS.NormalW = normalize(inputVS.NormalW);

    float3 toEyeW = normalize(gEyePosW - inputVS.PosW);

	// Start with a sum of zero. 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    // Sum the light contribution from each light source.
    float4 A, D, S;
    
    ComputeDirectionalLight(gMaterial, gDirLight, inputVS.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    ComputePointLight(gMaterial, gPointLight, inputVS.PosW, inputVS.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    ComputeSpotLight(gMaterial, gSpotLight, inputVS.PosW, inputVS.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
    
    float4 litColor = ambient + diffuse + spec;
    return litColor;
}


// FX
technique11 LightTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}