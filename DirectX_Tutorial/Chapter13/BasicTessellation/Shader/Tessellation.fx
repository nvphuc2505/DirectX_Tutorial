#include "D:/Work/DirectX/Chapter13/BasicTessellation/Shader/LightingHelper.fx"

cbuffer cbPerFrame
{
    DirectionalLight gDirLights[3];
    float3 gEyePosW;

    float gFogStart;
    float gFogRange;
    float4 gFogColor;
};

cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gWorldViewProj;
    float4x4 gTexTransform;
    Material gMaterial;
};



Texture2D gDiffuseMap;
SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    MaxAnisotropy = 4;

    AddressU = WRAP;
    AddressV = WRAP;
};



//
// Vertex Shader 
//
struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    vout.PosL = vin.PosL;
    
    return vout;
}



//
// Constant Hull Shader
//
struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch <VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    // Find the center of patch in world space
    float3 centerL = (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL) / 4.0f;
    float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
    float d = distance(centerW, gEyePosW);
    
    //
    // Tessellate the patch based on distance from the eye such that
	// the tessellation is 0 if d >= d1 and 60 if d <= d0.  The interval
	// [d0, d1] defines the range we tessellate in.
    //
    const float d0 = 20.0f;
    const float d1 = 100.0f;
    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));
    
    pt.EdgeTess[0] = tess;      // Left Edge
    pt.EdgeTess[1] = tess;      // Top Edge
    pt.EdgeTess[2] = tess;      // Right Edge
    pt.EdgeTess[3] = tess;      // Bottom Edge
    
    pt.InsideTess[0] = tess;    // u-axis
    pt.InsideTess[1] = tess;    // v-axis
    
    return pt;
}


//
// Control Point Hull Shader
//
struct HullOut
{
    float3 PosL : POSITION;
};


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> patch, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
    HullOut hout;

    hout.PosL = patch[i].PosL;
    
    return hout;
}



//
// Domain Shader
//
struct DomainOut
{
    float4 PosH : SV_POSITION;
};

[domain("quad")]
DomainOut DS(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;

    //
    // Bilinear Interpolation
    // Lerp(a, b, t) = a + (b - a) * t
    //
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 pos = lerp(v1, v2, uv.y);
    pos.y = 0.3f * (pos.z * sin(pos.x) + pos.x * cos(pos.z));
    
    dout.PosH = mul(float4(pos, 1.0f), gWorldViewProj);
    return dout;
}



//
// Pixel Shader
//
float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

technique11 Tess
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}