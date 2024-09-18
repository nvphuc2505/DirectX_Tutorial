#include "D:/Work/DirectX/Chapter11_GeometryShader/Shader/LightingHelper.fx"

// 
// Contant Buffers
//
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
    float4x4 gViewProj;
    Material gMaterial;
};

cbuffer cbFixed
{
    float2 gTexC[4] =
    {
        float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
    };
};



//
// Texture
//
Texture2DArray gTreeMapArray;
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};



struct VertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};



VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    vout.CenterW = vin.PosW;
    vout.SizeW = vin.SizeW;
    
    return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut vout[1], uint primID : SV_PrimitiveID, inout TriangleStream<GeoOut> triangleStream)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - vout[0].CenterW;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up, look);

    
    //
	// Compute triangle strip vertices (quad) in world space.
	//
    float halfWidth = 0.5f * vout[0].SizeW.x;
    float halfHeight = 0.5f * vout[0].SizeW.y;
  
    float4 vertex[4];
    vertex[0] = float4(vout[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
    vertex[1] = float4(vout[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
    vertex[2] = float4(vout[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
    vertex[3] = float4(vout[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);
    
    
    //
	// Transform quad vertices to world space and output them as a triangle strip.
	//
    GeoOut gout;
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        gout.PosH = mul(vertex[i], gViewProj);
        gout.PosW = vertex[i].xyz;
        gout.NormalW = look;
        gout.Tex = gTexC[i];
        gout.PrimID = primID;
        
        triangleStream.Append(gout);
    }
}

float4 PS(GeoOut gOut, uniform int gLightCount, uniform bool gUseTexure, uniform bool gAlphaClip, uniform bool gFogEnabled) : SV_Target
{
    gOut.NormalW = normalize(gOut.NormalW);
    
    float3 toEye = gEyePosW - gOut.PosW;

    float distanceToEye = length(toEye);
    
    toEye /= distanceToEye;
    
    
    //
    // Texturing
    //
    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (gUseTexure)
    {
        float uvw = float3(gOut.Tex, gOut.PrimID % 4);
        texColor = gTreeMapArray.Sample(samLinear, uvw);

        if (gAlphaClip)
            clip(texColor.a - 0.05f);

    }
    
    //
    // Lighting
    //
    float4 litColor = texColor;
    if (gLightCount > 0)
    {
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

        [uroll]
        for (int i = 0; i < gLightCount; i++)
        {
            float4 A, D, S;
            ComputeDirectionalLight(gMaterial, gDirLights[i], gOut.NormalW, toEye, A, D, S);
            
            ambient += A;
            diffuse += D;
            specular += S;
        }

        litColor = texColor * (ambient + diffuse) + specular;
    }

    
    //
    // Fogging
    //
    if (gFogEnabled)
    {
        float fogLerp = saturate((distanceToEye - gFogStart) / gFogRange);
        litColor = lerp(litColor, gFogColor, fogLerp);

    }

    return litColor;
}



//
// Effects
//
technique11 Light3
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS(3, false, false, false)));
    }
}

technique11 Light3TexAlphaClip
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, false)));
    }
}
            
technique11 Light3TexAlphaClipFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, true)));
    }
}