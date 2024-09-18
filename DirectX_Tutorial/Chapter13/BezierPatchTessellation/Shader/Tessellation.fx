#include "D:/Work/DirectX/Chapter13/BezierPatchTessellation/Shader/LightingHelper.fx"

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

PatchTess ConstantHS(InputPatch<VertexOut, 16> patch, uint patchID : SV_PrimitiveID)
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
    
    pt.EdgeTess[0] = tess; // Left Edge
    pt.EdgeTess[1] = tess; // Top Edge
    pt.EdgeTess[2] = tess; // Right Edge
    pt.EdgeTess[3] = tess; // Bottom Edge
    
    pt.InsideTess[0] = tess; // u-axis
    pt.InsideTess[1] = tess; // v-axis
    
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
[outputcontrolpoints(16)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 16> patch, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
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


float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;
    
    return float4(invT * invT * invT, 
            3.0f * t * invT * invT,
            3.0f * t * t * invT,
            t * t * t);
}

float3 CubicBezierSum(const OutputPatch<HullOut, 16> bezierPatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    
    sum = basisV.x * (basisU.x * bezierPatch[0].PosL + basisU.y * bezierPatch[1].PosL + basisU.z * bezierPatch[2].PosL + basisU.w * bezierPatch[3].PosL);
    sum += basisV.y * (basisU.x * bezierPatch[4].PosL + basisU.y * bezierPatch[5].PosL + basisU.z * bezierPatch[6].PosL + basisU.w * bezierPatch[7].PosL);
    sum += basisV.z * (basisU.x * bezierPatch[8].PosL + basisU.y * bezierPatch[9].PosL + basisU.z * bezierPatch[10].PosL + basisU.w * bezierPatch[11].PosL);
    sum += basisV.w * (basisU.x * bezierPatch[12].PosL + basisU.y * bezierPatch[13].PosL + basisU.z * bezierPatch[14].PosL + basisU.w * bezierPatch[15].PosL);
    
    return sum;
}


[domain("quad")]
DomainOut DS(PatchTess patchTess,
             float2 uv : SV_DomainLocation,
             const OutputPatch<HullOut, 16> bezPatch)
{
    DomainOut dout;
	
    float4 basisU = BernsteinBasis(uv.x);
    float4 basisV = BernsteinBasis(uv.y);

    float3 p = CubicBezierSum(bezPatch, basisU, basisV);
	
    dout.PosH = mul(float4(p, 1.0f), gWorldViewProj);
	
    return dout;
}



//
// Pixel Shader
//
float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}



//
// Effects
//
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